/**
 * Author: Soumyadeep Dash
 * Last Edited: 08/08/24
 */

#include "fft/fft.h"
#include <cmath>
#include <numbers>
#include <cstdlib>
#include <iostream>

using f64 = f32;

void impl::prepare_scalar(f64*& wr, f64*& wi, usize& size, const usize k)
{
    if (k == 0) return;
    const usize m = 1ULL << (k - 1);
    if (size >= m) return;
    size = m;
    std::free(wr);
    std::free(wi);
    wr = static_cast<f64*>(std::malloc(m * sizeof(f64)));
    wi = static_cast<f64*>(std::malloc(m * sizeof(f64)));
    if (wr == nullptr || wi == nullptr)
    {
        std::println(std::cerr, "Memory allocation failed");
        std::exit(1);
    }
    const f64 arg = static_cast<f64>(-std::numbers::pi / static_cast<f64>(m));
    wr[0] = 1.0, wi[0] = 0.0;
    for (usize i = 1, j = m >> 1; j; i <<= 1, j >>= 1)
    {
        const f64 theta = arg * static_cast<f64>(j);
        wr[i] = std::cos(theta);
        wi[i] = std::sin(theta);
    }
    for (usize i = 1; i < m; ++i)
    {
        const usize p = i & (i - 1), q = 1ULL << __builtin_ctzll(i);
        wr[i] = wr[p] * wr[q] - wi[p] * wi[q];
        wi[i] = wr[p] * wi[q] + wi[p] * wr[q];
    }
}

// -------------------------------------------------------------------------
// DIF (Decimation-In-Frequency) Forward FFT
//   Input:  natural order
//   Output: bit-reversed order
// -------------------------------------------------------------------------
void impl::dif_forward_scalar(const f64* wr, const f64* wi, f64* real, f64* imag, const usize k)
{
    if (k == 0) return;
    if (k == 1)
    {
        const f64 ur = real[0], vr = real[1];
        const f64 ui = imag[0], vi = imag[1];
        real[0] = ur + vr, real[1] = ur - vr;
        imag[0] = ui + vi, imag[1] = ui - vi;
        return;
    }
    if (k & 1)
    {
        for (usize i0 = 0, i1 = 1ULL << (k - 1), ie = i1; i0 < ie; ++i0, ++i1)
        {
            const f64 ur = real[i0], vr = real[i1];
            const f64 ui = imag[i0], vi = imag[i1];
            real[i0] = ur + vr, real[i1] = ur - vr;
            imag[i0] = ui + vi, imag[i1] = ui - vi;
        }
    }
    for (usize u = (k & 1) + 1, v = 1ULL << ((k & ~1ULL) - 2); v; u <<= 2, v >>= 2)
    {
        for (usize i0 = 0, i1 = v, i2 = i1 + v, i3 = i2 + v; i0 < v; ++i0, ++i1, ++i2, ++i3)
        {
            const f64 x0r = real[i0] + real[i2], x0i = imag[i0] + imag[i2];
            const f64 x1r = real[i1] + real[i3], x1i = imag[i1] + imag[i3];
            const f64 x2r = real[i0] - real[i2], x2i = imag[i0] - imag[i2];
            const f64 x3r = imag[i1] - imag[i3], x3i = real[i3] - real[i1];

            real[i0] = x0r + x1r, imag[i0] = x0i + x1i;
            real[i1] = x0r - x1r, imag[i1] = x0i - x1i;
            real[i2] = x2r + x3r, imag[i2] = x2i + x3i;
            real[i3] = x2r - x3r, imag[i3] = x2i - x3i;
        }
        for (usize h = 1; h < u; ++h)
        {
            const f64 w1r = wr[h << 1], w1i = wi[h << 1];
            const f64 w2r = wr[h], w2i = wi[h];
            const f64 w3r = w1r * w2r - w1i * w2i, w3i = w1r * w2i + w1i * w2r;

            for (usize i0 = h * 4 * v, i1 = i0 + v, i2 = i1 + v, i3 = i2 + v, ie = i1; i0 < ie; ++i0, ++i1, ++i2, ++i3)
            {
                const f64 t0r = real[i0], t0i = imag[i0];
                const f64 t1r = real[i1] * w1r - imag[i1] * w1i, t1i = real[i1] * w1i + imag[i1] * w1r;
                const f64 t2r = real[i2] * w2r - imag[i2] * w2i, t2i = real[i2] * w2i + imag[i2] * w2r;
                const f64 t3r = real[i3] * w3r - imag[i3] * w3i, t3i = real[i3] * w3i + imag[i3] * w3r;

                const f64 x0r = t0r + t2r, x0i = t0i + t2i;
                const f64 x1r = t1r + t3r, x1i = t1i + t3i;
                const f64 x2r = t0r - t2r, x2i = t0i - t2i;
                const f64 x3r = t1i - t3i, x3i = t3r - t1r;

                real[i0] = x0r + x1r, imag[i0] = x0i + x1i;
                real[i1] = x0r - x1r, imag[i1] = x0i - x1i;
                real[i2] = x2r + x3r, imag[i2] = x2i + x3i;
                real[i3] = x2r - x3r, imag[i3] = x2i - x3i;
            }
        }
    }
}

// -------------------------------------------------------------------------
// DIF (Decimation-In-Frequency) Inverse FFT
//   Input:  natural order
//   Output: bit-reversed order (NOT normalized — caller divides by N)
// -------------------------------------------------------------------------
void impl::dif_inverse_scalar(const f64* wr, const f64* wi, f64* real, f64* imag, const usize k)
{
    if (k == 0) return;
    if (k == 1)
    {
        const f64 ur = real[0], vr = real[1];
        const f64 ui = imag[0], vi = imag[1];
        real[0] = ur + vr, real[1] = ur - vr;
        imag[0] = ui + vi, imag[1] = ui - vi;
        return;
    }
    if (k & 1)
    {
        for (usize i0 = 0, i1 = 1ULL << (k - 1), ie = i1; i0 < ie; ++i0, ++i1)
        {
            const f64 ur = real[i0], vr = real[i1];
            const f64 ui = imag[i0], vi = imag[i1];
            real[i0] = ur + vr, real[i1] = ur - vr;
            imag[i0] = ui + vi, imag[i1] = ui - vi;
        }
    }
    for (usize u = (k & 1) + 1, v = 1ULL << ((k & ~1ULL) - 2); v; u <<= 2, v >>= 2)
    {
        for (usize i0 = 0, i1 = v, i2 = 2 * v, i3 = 3 * v; i0 < v; ++i0, ++i1, ++i2, ++i3)
        {
            const f64 x0r = real[i0] + real[i2], x0i = imag[i0] + imag[i2];
            const f64 x1r = real[i1] + real[i3], x1i = imag[i1] + imag[i3];
            const f64 x2r = real[i0] - real[i2], x2i = imag[i0] - imag[i2];
            const f64 x3r = imag[i3] - imag[i1], x3i = real[i1] - real[i3];

            real[i0] = x0r + x1r, imag[i0] = x0i + x1i;
            real[i1] = x0r - x1r, imag[i1] = x0i - x1i;
            real[i2] = x2r + x3r, imag[i2] = x2i + x3i;
            real[i3] = x2r - x3r, imag[i3] = x2i - x3i;
        }
        for (usize h = 1; h < u; ++h)
        {
            const f64 w1r = wr[h << 1], w1i = -wi[h << 1];
            const f64 w2r = wr[h], w2i = -wi[h];
            const f64 w3r = w1r * w2r - w1i * w2i, w3i = w1r * w2i + w1i * w2r;

            for (usize i0 = h * 4 * v, i1 = i0 + v, i2 = i1 + v, i3 = i2 + v, ie = i1; i0 < ie; ++i0, ++i1, ++i2, ++i3)
            {
                const f64 t0r = real[i0], t0i = imag[i0];
                const f64 t1r = real[i1] * w1r - imag[i1] * w1i, t1i = real[i1] * w1i + imag[i1] * w1r;
                const f64 t2r = real[i2] * w2r - imag[i2] * w2i, t2i = real[i2] * w2i + imag[i2] * w2r;
                const f64 t3r = real[i3] * w3r - imag[i3] * w3i, t3i = real[i3] * w3i + imag[i3] * w3r;

                const f64 x0r = t0r + t2r, x0i = t0i + t2i;
                const f64 x1r = t1r + t3r, x1i = t1i + t3i;
                const f64 x2r = t0r - t2r, x2i = t0i - t2i;
                const f64 x3r = t3i - t1i, x3i = t1r - t3r;

                real[i0] = x0r + x1r, imag[i0] = x0i + x1i;
                real[i1] = x0r - x1r, imag[i1] = x0i - x1i;
                real[i2] = x2r + x3r, imag[i2] = x2i + x3i;
                real[i3] = x2r - x3r, imag[i3] = x2i - x3i;
            }
        }
    }
}

// -------------------------------------------------------------------------
// DIT (Decimation-In-Time) Forward FFT
//   Input: bit-reversed order
//   Output: natural order
// -------------------------------------------------------------------------
void impl::dit_forward_scalar(const f64* wr, const f64* wi, f64* real, f64* imag, const usize k)
{
    if (k == 0) return;
    if (k == 1)
    {
        const f64 ur = real[0], vr = real[1];
        const f64 ui = imag[0], vi = imag[1];
        real[0] = ur + vr, real[1] = ur - vr;
        imag[0] = ui + vi, imag[1] = ui - vi;
        return;
    }
    for (usize u = 1ULL << (k - 2), v = 1; u; u >>= 2, v <<= 2)
    {
        for (usize i0 = 0, i1 = v, i2 = 2 * v, i3 = 3 * v; i0 < v; ++i0, ++i1, ++i2, ++i3)
        {
            const f64 x0r = real[i0] + real[i1], x0i = imag[i0] + imag[i1];
            const f64 x1r = real[i0] - real[i1], x1i = imag[i0] - imag[i1];
            const f64 x2r = real[i2] + real[i3], x2i = imag[i2] + imag[i3];
            const f64 x3r = imag[i2] - imag[i3], x3i = real[i3] - real[i2];

            real[i0] = x0r + x2r, imag[i0] = x0i + x2i;
            real[i1] = x1r + x3r, imag[i1] = x1i + x3i;
            real[i2] = x0r - x2r, imag[i2] = x0i - x2i;
            real[i3] = x1r - x3r, imag[i3] = x1i - x3i;
        }
        for (usize h = 1; h < u; ++h)
        {
            const f64 w1r = wr[h << 1], w1i = wi[h << 1];
            const f64 w2r = wr[h], w2i = wi[h];
            const f64 w3r = w1r * w2r - w1i * w2i, w3i = w1r * w2i + w1i * w2r;
            for (usize i0 = h * 4 * v, i1 = i0 + v, i2 = i1 + v, i3 = i2 + v, ie = i1; i0 < ie; ++i0, ++i1, ++i2, ++i3)
            {
                const f64 x0r = real[i0] + real[i1], x0i = imag[i0] + imag[i1];
                const f64 x1r = real[i0] - real[i1], x1i = imag[i0] - imag[i1];
                const f64 x2r = real[i2] + real[i3], x2i = imag[i2] + imag[i3];
                const f64 x3r = imag[i2] - imag[i3], x3i = real[i3] - real[i2];

                const f64 y0r = x0r + x2r, y0i = x0i + x2i;
                const f64 y1r = x1r + x3r, y1i = x1i + x3i;
                const f64 y2r = x0r - x2r, y2i = x0i - x2i;
                const f64 y3r = x1r - x3r, y3i = x1i - x3i;

                real[i0] = y0r, imag[i0] = y0i;
                real[i1] = y1r * w1r - y1i * w1i, imag[i1] = y1r * w1i + y1i * w1r;
                real[i2] = y2r * w2r - y2i * w2i, imag[i2] = y2r * w2i + y2i * w2r;
                real[i3] = y3r * w3r - y3i * w3i, imag[i3] = y3r * w3i + y3i * w3r;
            }
        }
    }
    if (k & 1)
    {
        for (usize i0 = 0, i1 = 1ULL << (k - 1), ie = i1; i0 < ie; ++i0, ++i1)
        {
            const f64 ur = real[i0], vr = real[i1];
            const f64 ui = imag[i0], vi = imag[i1];
            real[i0] = ur + vr, imag[i0] = ui + vi;
            real[i1] = ur - vr, imag[i1] = ui - vi;
        }
    }
}

// -------------------------------------------------------------------------
// DIT (Decimation-In-Time) Inverse FFT
//   Input:  bit-reversed order
//   Output: natural order (NOT normalized — caller divides by N)
// -------------------------------------------------------------------------
void impl::dit_inverse_scalar(const f64* wr, const f64* wi, f64* real, f64* imag, const usize k)
{
    if (k == 0) return;
    if (k == 1)
    {
        const f64 ur = real[0], vr = real[1];
        const f64 ui = imag[0], vi = imag[1];
        real[0] = ur + vr;
        real[1] = ur - vr;
        imag[0] = ui + vi;
        imag[1] = ui - vi;
        return;
    }
    for (usize u = 1ULL << (k - 2), v = 1; u; u >>= 2, v <<= 2)
    {
        for (usize i0 = 0, i1 = v, i2 = i1 + v, i3 = i2 + v; i0 < v; ++i0, ++i1, ++i2, ++i3)
        {
            const f64 x0r = real[i0] + real[i1], x0i = imag[i0] + imag[i1];
            const f64 x1r = real[i0] - real[i1], x1i = imag[i0] - imag[i1];
            const f64 x2r = real[i2] + real[i3], x2i = imag[i2] + imag[i3];
            const f64 x3r = imag[i3] - imag[i2], x3i = real[i2] - real[i3];

            real[i0] = x0r + x2r, imag[i0] = x0i + x2i;
            real[i1] = x1r + x3r, imag[i1] = x1i + x3i;
            real[i2] = x0r - x2r, imag[i2] = x0i - x2i;
            real[i3] = x1r - x3r, imag[i3] = x1i - x3i;
        }
        for (usize h = 1; h < u; ++h)
        {
            const f64 w1r = wr[h << 1], w1i = -wi[h << 1], w2r = wr[h], w2i = -wi[h];
            const f64 w3r = w1r * w2r - w1i * w2i, w3i = w1r * w2i + w1i * w2r;

            for (usize i0 = h * 4 * v, i1 = i0 + v, i2 = i1 + v, i3 = i2 + v, ie = i1; i0 < ie; ++i0, ++i1, ++i2, ++
                 i3)
            {
                const f64 x0r = real[i0] + real[i1], x0i = imag[i0] + imag[i1];
                const f64 x1r = real[i0] - real[i1], x1i = imag[i0] - imag[i1];
                const f64 x2r = real[i2] + real[i3], x2i = imag[i2] + imag[i3];
                const f64 x3r = imag[i3] - imag[i2], x3i = real[i2] - real[i3];

                const f64 y0r = x0r + x2r, y0i = x0i + x2i;
                const f64 y1r = x1r + x3r, y1i = x1i + x3i;
                const f64 y2r = x0r - x2r, y2i = x0i - x2i;
                const f64 y3r = x1r - x3r, y3i = x1i - x3i;

                real[i0] = y0r, imag[i0] = y0i;
                real[i1] = y1r * w1r - y1i * w1i, imag[i1] = y1r * w1i + y1i * w1r;
                real[i2] = y2r * w2r - y2i * w2i, imag[i2] = y2r * w2i + y2i * w2r;
                real[i3] = y3r * w3r - y3i * w3i, imag[i3] = y3r * w3i + y3i * w3r;
            }
        }
    }
    if (k & 1)
    {
        for (usize i0 = 0, i1 = 1ULL << (k - 1), ie = i1; i0 < ie; ++i0, ++i1)
        {
            const f64 ur = real[i0], vr = real[i1];
            const f64 ui = imag[i0], vi = imag[i1];
            real[i0] = ur + vr, imag[i0] = ui + vi;
            real[i1] = ur - vr, imag[i1] = ui - vi;
        }
    }
}
