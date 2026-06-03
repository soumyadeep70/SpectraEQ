#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstring>
#include <sstream>
#include "miniaudio/miniaudio.h"
#include "fft/fft.h"
#include "audio/audio_processor.h"


AUDIO_PROCESSOR::AUDIO_PROCESSOR()
{
    if (ma_context_init(nullptr, 0, nullptr, &m_context) != MA_SUCCESS) {
        std::println(std::cerr, "Failed to initialize miniaudio context.");
        std::exit(-1);
    }
    m_audio_buf.resize(BUF_SIZE, 0);
    m_buf_write_index = 0;
    m_isDeviceInitialized = false;
    refresh_available_devices();

    m_hann_window.resize(FFT_SIZE);
    for (size_t i = 0; i < FFT_SIZE; ++i)
        m_hann_window[i] = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * M_PI * static_cast<float>(i) / (FFT_SIZE - 1))));

    fft.prepare(FFT_SIZE_LG);
    m_fft_real.resize(FFT_SIZE, 0.0f);
    m_fft_imag.resize(FFT_SIZE, 0.0f);
}

AUDIO_PROCESSOR::~AUDIO_PROCESSOR()
{
    if (m_isDeviceInitialized)
    {
        ma_device_stop(&m_device);
        ma_device_uninit(&m_device);
    }
    ma_context_uninit(&m_context);
}

void AUDIO_PROCESSOR::refresh_available_devices()
{
    size_t default_device_index = 0;
    {
        std::lock_guard lock(m_mutex);
        ma_device_info* pCaptureDeviceInfos;
        ma_uint32 captureDeviceCount;
        if (ma_context_get_devices(&m_context, nullptr, nullptr, &pCaptureDeviceInfos, &captureDeviceCount) != MA_SUCCESS) {
            println(std::cerr, "Failed to retrieve device information.");
            std::exit(-1);
        }
        m_available_devices.assign(pCaptureDeviceInfos, pCaptureDeviceInfos + captureDeviceCount);
        for (size_t i = 0; i < m_available_devices.size(); ++i)
        {
            if (m_available_devices[i].isDefault == MA_FALSE) continue;
            default_device_index = i;
            break;
        }
    }
    set_selected_device_index(default_device_index);
}

void AUDIO_PROCESSOR::set_selected_device_index(const size_t index)
{
    if (index >= m_available_devices.size())
    {
        std::println(std::cerr, "No devices found at index {}.", index);
        return;
    }
    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format = ma_format_f32;
    config.capture.channels = 1;
    config.capture.pDeviceID = &m_available_devices[index].id;
    config.pUserData = this;
    config.dataCallback = [](ma_device* pDevice, void* /*pOutput*/, const void* pInput, const ma_uint32 frameCount)
    {
        const auto self = static_cast<AUDIO_PROCESSOR*>(pDevice->pUserData);
        const auto pInputBuf = static_cast<const float*>(pInput);
        if (BUF_SIZE - self->m_buf_write_index >= frameCount)
        {
            std::memcpy(self->m_audio_buf.data() + self->m_buf_write_index, pInputBuf, frameCount * sizeof(float));
        } else
        {
            const size_t rem = BUF_SIZE - self->m_buf_write_index;
            std::memcpy(self->m_audio_buf.data() + self->m_buf_write_index, pInputBuf, rem * sizeof(float));
            std::memcpy(self->m_audio_buf.data(), pInputBuf + rem, (frameCount - rem) * sizeof(float));
        }
        self->m_buf_write_index += frameCount;
        if (self->m_buf_write_index >= BUF_SIZE) self->m_buf_write_index -= BUF_SIZE;
    };
    {
        std::lock_guard lock(m_mutex);
        if (m_isDeviceInitialized) {
            ma_device_stop(&m_device);
            ma_device_uninit(&m_device);
        }
        if (ma_device_init(nullptr, &config, &m_device) != MA_SUCCESS) {
            std::println(std::cerr, "Failed to initialize miniaudio device.");
            std::exit(-1);
        }
        m_isDeviceInitialized = true;
        m_selected_device_index = index;
        ma_device_start(&m_device);
    }
}

std::vector<ma_device_info> AUDIO_PROCESSOR::get_available_devices() const
{
    std::lock_guard lock(m_mutex);
    return m_available_devices;
}

size_t AUDIO_PROCESSOR::get_selected_device_index() const
{
    std::lock_guard lock(m_mutex);
    return m_selected_device_index;
}

uint32_t AUDIO_PROCESSOR::get_device_sample_rate() const
{
    std::lock_guard lock(m_mutex);
    return m_device.sampleRate;
}

std::vector<float> AUDIO_PROCESSOR::get_frequency_bands(
    const float min_hz,
    const float max_hz,
    const size_t bin_count)
{
    float sampleRate = 0.0f;
    {
        std::lock_guard lock(m_mutex);
        sampleRate = static_cast<float>(m_device.sampleRate);
    }
    // 1. Build a logarithmic frequency map
    // TODO: cache the band mapping
    std::vector<std::pair<size_t, size_t>> bands(bin_count);
    const float log_min = std::log(min_hz);
    const float log_max = std::log(max_hz);
    constexpr size_t num_bins  = FFT_SIZE / 2;
    const float hz_per_bin = sampleRate / FFT_SIZE;

    for (size_t col = 0; col < bin_count; ++col) {
        const float lo_hz = std::exp(log_min + (log_max - log_min) *  static_cast<float>(col) / static_cast<float>(bin_count));
        const float hi_hz = std::exp(log_min + (log_max - log_min) * static_cast<float>(col + 1) / static_cast<float>(bin_count));

        size_t lo_bin = std::max(1UL, static_cast<size_t>(lo_hz / hz_per_bin));
        size_t hi_bin = std::min(num_bins - 1, static_cast<size_t>(hi_hz / hz_per_bin));
        if (hi_bin < lo_bin) hi_bin = lo_bin;

        bands[col] = {lo_bin, hi_bin};
    }

    // 2. Grab audio from ring buffer, apply hann window and perform fft
    const size_t current_write_index = m_buf_write_index.load(std::memory_order_relaxed);
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        const size_t idx = (current_write_index + BUF_SIZE - FFT_SIZE + i) % BUF_SIZE;
        m_fft_real[i] = m_audio_buf[idx] * m_hann_window[i];
        m_fft_imag[i] = 0.0f;
    }

    // 3. Perform FFT
    fft.dif_forward(m_fft_real.data(), m_fft_imag.data(), FFT_SIZE_LG);
    FFT::bit_reverse_order(m_fft_real.data(), m_fft_imag.data(), FFT_SIZE_LG);

    // 4. Compute log-scaled bar magnitudes
    std::vector<float> freq_bands(bin_count, 0.0);
    for (size_t col = 0; col < bin_count; ++col) {
        auto [lo, hi] = bands[col];
        float peak = 0.0f;
        for (size_t b = lo; b <= hi; ++b) {
            float mag = std::sqrt(m_fft_real[b] * m_fft_real[b] + m_fft_imag[b] * m_fft_imag[b]);
            mag /= (FFT_SIZE / 2.0f); // normalize
            if (mag > peak) peak = mag;
        }

        // Convert to dB for a more musical response, then map to height
        // dB range: -60 dB (silence) to 0 dB (full scale)
        const float db = 20.0f * std::log10(peak + 1e-6f);
        float normalized = (db + 60.0f) / 60.0f; // 0→1 over -60..0 dB
        normalized = std::max(0.0f, std::min(1.0f, normalized));
        freq_bands[col] = normalized;
    }

    return freq_bands;
}