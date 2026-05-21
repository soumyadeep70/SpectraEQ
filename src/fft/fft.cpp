#include "fft/fft.h"

#include <cstdlib>
#include <vector>

FFT::FFT() : wr(nullptr), wi(nullptr), size(0) {
    // __builtin_cpu_init();
    // const bool FMA_SUPPORTED = __builtin_cpu_supports("fma");
    // const bool AVX_SUPPORTED = __builtin_cpu_supports("avx");
    // const bool SSE2_SUPPORTED = __builtin_cpu_supports("sse2");
    // if (FMA_SUPPORTED) {
    //     _reserve_ = FFT_AVX_FMA::reserve;
    //     _fft_ = FFT_AVX_FMA::fft;
    //     _ifft_ = FFT_AVX_FMA::ifft;
    //     _cyclic_conv_ = FFT_AVX_FMA::cyclic_conv;
    // } else if (AVX_SUPPORTED) {
    //     _reserve_ = FFT_AVX::reserve;
    //     _fft_ = FFT_AVX::fft;
    //     _ifft_ = FFT_AVX::ifft;
    //     _cyclic_conv_ = FFT_AVX::cyclic_conv;
    // } else if (SSE2_SUPPORTED) {
    //     _reserve_ = FFT_SSE2::reserve;
    //     _fft_ = FFT_SSE2::fft;
    //     _ifft_ = FFT_SSE2::ifft;
    //     _cyclic_conv_ = FFT_SSE2::cyclic_conv;
    // } else {
    //     _reserve_ = FFT_GENERIC::reserve;
    //     _fft_ = FFT_GENERIC::fft;
    //     _ifft_ = FFT_GENERIC::ifft;
    //     _cyclic_conv_ = FFT_GENERIC::cyclic_conv;
    // }
    prepare_fn = impl::prepare_scalar;
    dif_forward_fn = impl::dif_forward_scalar;
    dif_inverse_fn = impl::dif_inverse_scalar;
    dit_forward_fn = impl::dit_forward_scalar;
    dit_inverse_fn = impl::dit_inverse_scalar;
}

FFT::~FFT() {
    std::free(wr);
    std::free(wi);
}

void FFT::bit_reverse_order(f32* real, f32* imag, const usize k)
{
    if (k == 0) return;
    const usize n = 1ULL << k;
    for (usize i = 1, j = 0; i < n; i++) {
        usize bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) {
            std::swap(real[i], real[j]);
            std::swap(imag[i], imag[j]);
        }
    }
}

void FFT::prepare(const usize k)
{
    prepare_fn(wr, wi, size, k);
}

void FFT::dif_forward(f32 *real, f32 *imag, const usize k) const
{
    dif_forward_fn(wr, wi, real, imag, k);
}

void FFT::dif_inverse(f32 *real, f32 *imag, const usize k) const
{
    dif_inverse_fn(wr, wi, real, imag, k);
}

void FFT::dit_forward(f32 *real, f32 *imag, const usize k) const
{
    dit_forward_fn(wr, wi, real, imag, k);
}

void FFT::dit_inverse(f32 *real, f32 *imag, const usize k) const
{
    dit_inverse_fn(wr, wi, real, imag, k);
}
