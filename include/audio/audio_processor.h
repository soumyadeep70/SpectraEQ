#pragma once
#include <mutex>
#include <iostream>
#include <string>
#include <vector>
#include "miniaudio/miniaudio.h"
#include "fft/fft.h"

class AUDIO_PROCESSOR
{
public:
    AUDIO_PROCESSOR();
    ~AUDIO_PROCESSOR();
    void refresh_available_devices();
    void set_selected_device_index(size_t index);

    [[nodiscard]] std::vector<ma_device_info> get_available_devices() const;
    [[nodiscard]] size_t get_selected_device_index() const;
    [[nodiscard]] uint32_t get_device_sample_rate() const;
    [[nodiscard]] std::vector<float> get_frequency_bands(float min_hz, float max_hz, size_t bin_count);

private:
    static constexpr size_t BUF_SIZE = 48000;
    static constexpr size_t FFT_SIZE = 2048;
    static_assert(std::has_single_bit(FFT_SIZE));
    static constexpr size_t FFT_SIZE_LG = std::countr_zero(FFT_SIZE);

    mutable std::mutex m_mutex;
    ma_context m_context;
    std::vector<ma_device_info> m_available_devices;
    size_t m_selected_device_index;
    ma_device m_device;
    bool m_isDeviceInitialized;
    std::vector<float> m_audio_buf;
    std::atomic_size_t m_buf_write_index;

    std::vector<float> m_hann_window;
    FFT fft;
    std::vector<float> m_fft_real, m_fft_imag;
};