#include "miniaudio/miniaudio.h"
#include "fft/fft.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <mutex>
#include <thread>
#include <chrono>
#include <csignal>
#include <sstream>

constexpr int FFT_SIZE_LG  = 11;
constexpr int FFT_SIZE     = 1 << FFT_SIZE_LG; // 2048 for better frequency resolution
constexpr int TERMINAL_WIDTH  = 80;
constexpr int TERMINAL_HEIGHT = 24;

// ── Color palette: cyan → green → yellow → red (bottom→top) ──────────────────
// We'll pick one of these ANSI 256-color ramp indices per row.
// Row 0 = bottom (low intensity), row TERMINAL_HEIGHT-1 = top (peak).
static const char* bar_char = "█";

// Returns an ANSI escape that sets foreground to a gradient color.
// fraction: 0.0 (bottom/cool) → 1.0 (top/hot)
inline std::string bar_color(float fraction) {
    // Map fraction → hue: 180° (cyan) → 120° (green) → 60° (yellow) → 0° (red)
    // We'll use xterm-256 palette indices for a smooth ramp.
    // The 6x6x6 color cube starts at index 16.
    // We pick colors along the gradient: cyan(51) → green(46) → yellow(226) → red(196)
    // Approximate with a linear interpolation through those waypoints.

    struct Waypoint { float pos; int r, g, b; };
    static const Waypoint stops[] = {
        {0.00f,  0, 220, 180},  // teal/cyan
        {0.33f,  0, 255,  60},  // bright green
        {0.60f, 220, 220,   0},  // yellow
        {0.80f, 255, 120,   0},  // orange
        {1.00f, 255,   0,   0},  // red
    };
    constexpr int N = sizeof(stops)/sizeof(stops[0]);

    // Find segment
    int seg = 0;
    for (int i = 0; i < N-1; ++i) {
        if (fraction <= stops[i+1].pos) { seg = i; break; }
        seg = i;
    }
    if (fraction >= 1.0f) seg = N-2;

    float t = (fraction - stops[seg].pos) / (stops[seg+1].pos - stops[seg].pos + 1e-6f);
    t = std::max(0.0f, std::min(1.0f, t));

    int r = (int)(stops[seg].r + t * (stops[seg+1].r - stops[seg].r));
    int g = (int)(stops[seg].g + t * (stops[seg+1].g - stops[seg].g));
    int b = (int)(stops[seg].b + t * (stops[seg+1].b - stops[seg].b));

    // Emit a 24-bit foreground color escape
    char buf[32];
    snprintf(buf, sizeof(buf), "\033[38;2;%d;%d;%dm", r, g, b);
    return std::string(buf);
}

// ─────────────────────────────────────────────────────────────────────────────

std::vector<float> global_audio_buffer(FFT_SIZE, 0.0f);
std::mutex audio_mutex;
int write_index = 0;
bool running = true;

void handle_sigint(int /*sig*/) { running = false; }

void data_callback(ma_device* /*pDevice*/, void* /*pOutput*/,
                   const void* pInput, ma_uint32 frameCount)
{
    const float* pInputF = (const float*)pInput;
    std::lock_guard<std::mutex> lock(audio_mutex);
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        global_audio_buffer[write_index] = (pInputF[i*2] + pInputF[i*2+1]) * 0.5f;
        write_index = (write_index + 1) % FFT_SIZE;
    }
}

// Build a logarithmic frequency map: each of the TERMINAL_WIDTH columns covers
// a frequency band whose edges are logarithmically spaced between min_hz and max_hz.
// Returns pairs of (start_bin, end_bin) inclusive for each column.
static std::vector<std::pair<int,int>> build_log_bands(int num_cols,
                                                        float sample_rate,
                                                        float min_hz = 40.0f,
                                                        float max_hz = 16000.0f)
{
    std::vector<std::pair<int,int>> bands(num_cols);
    float log_min = std::log(min_hz);
    float log_max = std::log(max_hz);
    int num_bins  = FFT_SIZE / 2;
    float hz_per_bin = sample_rate / FFT_SIZE;

    for (int col = 0; col < num_cols; ++col) {
        float lo_hz = std::exp(log_min + (log_max - log_min) *  col      / num_cols);
        float hi_hz = std::exp(log_min + (log_max - log_min) * (col + 1) / num_cols);

        int lo_bin = std::max(1, (int)(lo_hz / hz_per_bin));
        int hi_bin = std::min(num_bins - 1, (int)(hi_hz / hz_per_bin));
        if (hi_bin < lo_bin) hi_bin = lo_bin;

        bands[col] = {lo_bin, hi_bin};
    }
    return bands;
}

int main() {
    std::signal(SIGINT, handle_sigint);

    // ── Init miniaudio ────────────────────────────────────────────────────────
    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format   = ma_format_f32;
    config.capture.channels = 2;
    config.sampleRate        = 44100;
    config.dataCallback      = data_callback;

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        std::cerr << "Failed to initialize miniaudio.\n";
        return -1;
    }
    ma_device_start(&device);

    // ── Init FFT ──────────────────────────────────────────────────────────────
    FFT fft;
    fft.prepare(FFT_SIZE_LG);
    std::vector<float> fft_real(FFT_SIZE), fft_imag(FFT_SIZE);

    // ── Pre-compute log-scale frequency bands ─────────────────────────────────
    auto bands = build_log_bands(TERMINAL_WIDTH, 44100.0f);

    // ── Terminal setup ────────────────────────────────────────────────────────
    std::cout << "\033[?1049h"  // alternate screen
              << "\033[2J"      // clear
              << "\033[?25l"    // hide cursor
              << std::flush;

    std::vector<float> smooth_bars(TERMINAL_WIDTH, 0.0f);

    // Pre-compute Hann window
    std::vector<float> hann(FFT_SIZE);
    for (int i = 0; i < FFT_SIZE; ++i)
        hann[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (FFT_SIZE - 1)));

    // ── Render loop ───────────────────────────────────────────────────────────
    while (running) {
        // 1. Grab audio from ring buffer
        std::vector<float> local_buf(FFT_SIZE);
        {
            std::lock_guard<std::mutex> lock(audio_mutex);
            for (int i = 0; i < FFT_SIZE; ++i) {
                int idx = (write_index - FFT_SIZE + i + FFT_SIZE) % FFT_SIZE;
                local_buf[i] = global_audio_buffer[idx];
            }
        }

        // 2. Apply Hann window
        for (int i = 0; i < FFT_SIZE; ++i) {
            fft_real[i] = local_buf[i] * hann[i];
            fft_imag[i] = 0.0f;
        }

        // 3. Forward DIF FFT (input: natural order → output: bit-reversed order)
        fft.dif_forward(fft_real.data(), fft_imag.data(), FFT_SIZE_LG);

        // 4. Un-scramble from bit-reversed → natural order so bin[k] = frequency k
        FFT::bit_reverse_order(fft_real.data(), fft_imag.data(), FFT_SIZE_LG);
        // Now fft_real[k] / fft_imag[k] holds the complex coefficient for bin k.

        // 5. Compute log-scaled bar magnitudes
        std::vector<float> bar_heights_f(TERMINAL_WIDTH, 0.0f);
        for (int col = 0; col < TERMINAL_WIDTH; ++col) {
            auto [lo, hi] = bands[col];
            float peak = 0.0f;
            for (int b = lo; b <= hi; ++b) {
                float mag = std::sqrt(fft_real[b]*fft_real[b] + fft_imag[b]*fft_imag[b]);
                mag /= (FFT_SIZE / 2.0f); // normalize
                if (mag > peak) peak = mag;
            }

            // Convert to dB for a more musical response, then map to height
            // dB range: -60 dB (silence) to 0 dB (full scale)
            float db = 20.0f * std::log10(peak + 1e-6f);
            float normalized = (db + 60.0f) / 60.0f; // 0→1 over -60..0 dB
            normalized = std::max(0.0f, std::min(1.0f, normalized));

            float target = normalized * TERMINAL_HEIGHT;

            // Temporal smoothing: fast attack, slow decay
            if (target > smooth_bars[col])
                smooth_bars[col] = smooth_bars[col] * 0.1f + target * 0.9f;
            else
                smooth_bars[col] = smooth_bars[col] * 0.85f + target * 0.15f;

            bar_heights_f[col] = smooth_bars[col];
        }

        // 6. Render ──────────────────────────────────────────────────────────
        // Build the entire frame in a string to minimize flicker
        std::ostringstream frame;
        frame << "\033[H"; // move cursor to top-left

        for (int row = TERMINAL_HEIGHT; row >= 1; --row) {
            for (int col = 0; col < TERMINAL_WIDTH; ++col) {
                if (bar_heights_f[col] >= (float)row) {
                    // Color fraction: how high up this filled cell is relative to bar top
                    float frac = (float)row / TERMINAL_HEIGHT;
                    frame << bar_color(frac) << bar_char;
                } else {
                    frame << "\033[0m "; // reset + space
                }
            }
            frame << "\033[0m\n";
        }

        // Footer
        std::string line;
        for (int i = 0; i < TERMINAL_WIDTH - 10; ++i)
            line += "─";
        frame << "\033[38;2;80;80;100m"
              << "  ◄ "
              << line
              << " ►  "
              << "\033[0m\n";
        frame << "\033[38;2;60;60;80m  SPECTRUM  │  Ctrl+C to exit\033[0m";

        std::cout << frame.str() << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    std::cout << "\033[?25h"    // show cursor
              << "\033[?1049l"  // exit alternate screen
              << std::flush;
    std::cout << "\nVisualizer closed.\n";

    ma_device_uninit(&device);
    return 0;
}