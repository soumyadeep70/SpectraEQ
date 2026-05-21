#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <random>

#include "fft/fft.h"
constexpr f32 EPS_BASE = 1e-7f;

class FFTScalarTest : public testing::Test {
protected:
    f32 *wr = nullptr;
    f32 *wi = nullptr;
    usize size = 0;

    void TearDown() override {
        free(wr);
        free(wi);
    }

    // Helper: generate random test signal
    static void GenerateRandom(std::vector<f32>& real, std::vector<f32>& imag, const usize n) {
        std::mt19937 gen(67);
        std::uniform_real_distribution dist(-1.0f, 1.0f);
        real.resize(n);
        imag.resize(n);
        for (usize i = 0; i < n; ++i) {
            real[i] = dist(gen);
            imag[i] = dist(gen);
        }
    }

    // Helper: Scale a vector by a constant (useful for the 1/N inverse scaling)
    static void ScaleVector(std::vector<f32>& vec, const f32 scale) {
        for (auto& val : vec) {
            val *= scale;
        }
    }

    // Helper: verify two arrays match within floating point tolerance
    static void ExpectArraysClose(const std::vector<f32>& actual, const std::vector<f32>& expected) {
        const usize n = actual.size();
        ASSERT_EQ(actual.size(), expected.size());
        for (size_t i = 0; i < n; ++i) {
            EXPECT_NEAR(actual[i], expected[i], EPS_BASE * static_cast<f32>(n)) << "Mismatch at index " << i;
        }
    }
};

// ---------------------------------------------------------
// 1. Prepare Function Tests
// ---------------------------------------------------------

TEST_F(FFTScalarTest, PrepareAllocatesCorrectly) {
    constexpr usize k = 15;
    impl::prepare_scalar(wr, wi, size, k);

    EXPECT_NE(wr, nullptr);
    EXPECT_NE(wi, nullptr);
    EXPECT_EQ(size, 1ULL << (k - 1));
}

// ---------------------------------------------------------
// 2. Decimation In Frequency (DIF) Tests
// ---------------------------------------------------------

TEST_F(FFTScalarTest, DifForward_DcSignal) {
    constexpr usize k = 15;
    constexpr usize n = 1 << k;
    impl::prepare_scalar(wr, wi, size, k);

    std::vector real(n, 1.0f);
    std::vector imag(n, 0.0f);

    impl::dif_forward_scalar(wr, wi, real.data(), imag.data(), k);

    constexpr f32 tolerance = EPS_BASE * static_cast<f32>(n);
    EXPECT_NEAR(real[0], static_cast<f32>(n), tolerance);
    EXPECT_NEAR(imag[0], 0.0f, tolerance);

    for (usize i = 1; i < n; ++i) {
        EXPECT_NEAR(real[i], 0.0f, tolerance);
        EXPECT_NEAR(imag[i], 0.0f, tolerance);
    }
}

TEST_F(FFTScalarTest, DifForwardAndInverse_Identity) {
    constexpr usize k = 17;
    constexpr usize n = 1 << k;
    impl::prepare_scalar(wr, wi, size, k);

    std::vector<f32> orig_real, orig_imag;
    GenerateRandom(orig_real, orig_imag, n);

    std::vector<f32> real = orig_real;
    std::vector<f32> imag = orig_imag;

    impl::dif_forward_scalar(wr, wi, real.data(), imag.data(), k);
    FFT::bit_reverse_order(real.data(), imag.data(), k);

    impl::dif_inverse_scalar(wr, wi, real.data(), imag.data(), k);
    FFT::bit_reverse_order(real.data(), imag.data(), k);

    ScaleVector(real, 1.0f / n);
    ScaleVector(imag, 1.0f / n);

    ExpectArraysClose(real, orig_real);
    ExpectArraysClose(imag, orig_imag);
}

// ---------------------------------------------------------
// 3. Decimation In Time (DIT) Tests
// ---------------------------------------------------------

TEST_F(FFTScalarTest, DitForward_DcSignal) {
    constexpr usize k = 15;
    constexpr usize n = 1 << k;
    impl::prepare_scalar(wr, wi, size, k);

    std::vector real(n, 1.0f);
    std::vector imag(n, 0.0f);

    impl::dit_forward_scalar(wr, wi, real.data(), imag.data(), k);

    constexpr f32 tolerance = EPS_BASE * static_cast<f32>(n);
    EXPECT_NEAR(real[0], static_cast<f32>(n), tolerance);
    EXPECT_NEAR(imag[0], 0.0f, tolerance);
    for (usize i = 1; i < n; ++i) {
        EXPECT_NEAR(real[i], 0.0f, tolerance);
        EXPECT_NEAR(imag[i], 0.0f, tolerance);
    }
}

TEST_F(FFTScalarTest, DitForwardAndInverse_Identity) {
    constexpr usize k = 17;
    constexpr usize n = 1 << k;
    impl::prepare_scalar(wr, wi, size, k);

    std::vector<f32> orig_real, orig_imag;
    GenerateRandom(orig_real, orig_imag, n);

    std::vector<f32> real = orig_real;
    std::vector<f32> imag = orig_imag;

    FFT::bit_reverse_order(real.data(), imag.data(), k);
    impl::dit_forward_scalar(wr, wi, real.data(), imag.data(), k);

    FFT::bit_reverse_order(real.data(), imag.data(), k);
    impl::dit_inverse_scalar(wr, wi, real.data(), imag.data(), k);

    ScaleVector(real, 1.0f / n);
    ScaleVector(imag, 1.0f / n);

    ExpectArraysClose(real, orig_real);
    ExpectArraysClose(imag, orig_imag);
}

// ---------------------------------------------------------
// 4. Cross-algorithm Equivalence (DIF vs DIT)
// ---------------------------------------------------------

TEST_F(FFTScalarTest, DifAndDitEquivalence) {
    constexpr usize k = 17;
    constexpr usize n = 1 << k;
    impl::prepare_scalar(wr, wi, size, k);

    std::vector<f32> input_real, input_imag;
    GenerateRandom(input_real, input_imag, n);

    std::vector<f32> dif_real = input_real;
    std::vector<f32> dif_imag = input_imag;
    impl::dif_forward_scalar(wr, wi, dif_real.data(), dif_imag.data(), k);
    FFT::bit_reverse_order(dif_real.data(), dif_imag.data(), k);

    std::vector<f32> dit_real = input_real;
    std::vector<f32> dit_imag = input_imag;
    FFT::bit_reverse_order(dit_real.data(), dit_imag.data(), k);
    impl::dit_forward_scalar(wr, wi, dit_real.data(), dit_imag.data(), k);

    ExpectArraysClose(dif_real, dit_real);
    ExpectArraysClose(dif_imag, dit_imag);
}