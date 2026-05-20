#pragma once

using usize = unsigned long long;
using f32 = float;

class FFT {
    f32 *wr, *wi;
    usize size;

    void (*_reserve_)(f32*&, f32*&, usize&, usize);
    void (*_dif_forward_)(const f32*, const f32*, f32*, f32*, usize);
    void (*_dit_inverse_)(const f32*, const f32*, f32*, f32*, usize);
    void (*_cyclic_conv_)(const f32*, const f32*, f32*, f32*, usize);

public:
    FFT();

    // input, output both in normal order
    void fft_forward(f32 *real, f32 *imag, usize k);
    void fft_inverse(f32 *real, f32 *imag, usize k);

    // input in normal, output in bit-reversed order
    void dif_forward(f32 *real, f32 *imag, usize k);
    void dif_forward_scalar(f32 *real, f32 *imag, usize k);
    void dif_forward_sse2(f32 *real, f32 *imag, usize k);
    void dif_forward_avx(f32 *real, f32 *imag, usize k);
    void dif_forward_avx_fma(f32 *real, f32 *imag, usize k);

    // input in bit-reversed, output in normal order
    void dit_forward(f32 *real, f32 *imag, usize k);
    void dit_forward_scalar(f32 *real, f32 *imag, usize k);
    void dit_forward_sse2(f32 *real, f32 *imag, usize k);
    void dit_forward_avx(f32 *real, f32 *imag, usize k);
    void dit_forward_avx_fma(f32 *real, f32 *imag, usize k);
};
