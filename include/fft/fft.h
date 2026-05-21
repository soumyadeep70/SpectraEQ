#pragma once

using usize = unsigned long long;
using f32 = float;

class FFT {
    f32 *wr, *wi;
    usize size;

    void (*prepare_fn)(f32*&, f32*&, usize&, usize);
    void (*dif_forward_fn)(const f32*, const f32*, f32*, f32*, usize);
    void (*dif_inverse_fn)(const f32*, const f32*, f32*, f32*, usize);
    void (*dit_forward_fn)(const f32*, const f32*, f32*, f32*, usize);
    void (*dit_inverse_fn)(const f32*, const f32*, f32*, f32*, usize);

public:
    FFT();
    ~FFT();
    FFT(const FFT&) = delete;
    FFT& operator=(const FFT&) = delete;

    static void bit_reverse_order(f32 *real, f32 *imag, usize k);
    void prepare(usize k);
    void dif_forward(f32*, f32*, usize) const;
    void dif_inverse(f32*, f32*, usize) const;
    void dit_forward(f32*, f32*, usize) const;
    void dit_inverse(f32*, f32*, usize) const;
};

namespace impl
{
    // reserve twiddles for the forward/inverse transforms
    void prepare_scalar(f32*& wr, f32*& wi, usize& size, usize k);
    void prepare_sse2(f32*& wr, f32*& wi, usize& size, usize k);
    void prepare_avx(f32*& wr, f32*& wi, usize& size, usize k);
    void prepare_fma(f32*& wr, f32*& wi, usize& size, usize k);

    // input in normal, output in bit-reversed order
    void dif_forward_scalar(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dif_forward_sse2(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dif_forward_avx(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dif_forward_fma(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);

    // input in normal, output in bit-reversed order
    void dif_inverse_scalar(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dif_inverse_sse2(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dif_inverse_avx(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dif_inverse_fma(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);

    // input in bit-reversed, output in normal order
    void dit_forward_scalar(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dit_forward_sse2(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dit_forward_avx(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dit_forward_fma(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);

    // input in bit-reversed, output in normal order
    void dit_inverse_scalar(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dit_inverse_sse2(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dit_inverse_avx(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
    void dit_inverse_fma(const f32* wr, const f32* wi, f32 *real, f32 *imag, usize k);
}
