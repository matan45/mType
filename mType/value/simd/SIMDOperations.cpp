#include "SIMDOperations.hpp"
#include <algorithm>
#include <limits>

namespace mType::value::simd
{
    // ========== INTEGER ADDITION ==========

    void SIMDOperations::addInt(const int* a, const int* b, int* result, size_t size)
    {
#if defined(MTYPE_SIMD_AVX2)
        // AVX2: Process 8 ints at once (256-bit)
        size_t simdSize = size / 8 * 8;
        for (size_t i = 0; i < simdSize; i += 8) {
            __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&a[i]));
            __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&b[i]));
            __m256i vr = _mm256_add_epi32(va, vb);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&result[i]), vr);
        }
        // Process remainder
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + b[i];
        }
#elif defined(MTYPE_SIMD_SSE2)
        // SSE2: Process 4 ints at once (128-bit)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&a[i]));
            __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&b[i]));
            __m128i vr = _mm_add_epi32(va, vb);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&result[i]), vr);
        }
        // Process remainder
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + b[i];
        }
#elif defined(MTYPE_SIMD_NEON)
        // NEON: Process 4 ints at once (128-bit)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            int32x4_t va = vld1q_s32(&a[i]);
            int32x4_t vb = vld1q_s32(&b[i]);
            int32x4_t vr = vaddq_s32(va, vb);
            vst1q_s32(&result[i], vr);
        }
        // Process remainder
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + b[i];
        }
#else
        // Scalar fallback
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] + b[i];
        }
#endif
    }

    void SIMDOperations::addScalarInt(const int* a, int scalar, int* result, size_t size)
    {
#if defined(MTYPE_SIMD_AVX2)
        __m256i vScalar = _mm256_set1_epi32(scalar);
        size_t simdSize = size / 8 * 8;
        for (size_t i = 0; i < simdSize; i += 8) {
            __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&a[i]));
            __m256i vr = _mm256_add_epi32(va, vScalar);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&result[i]), vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + scalar;
        }
#elif defined(MTYPE_SIMD_SSE2)
        __m128i vScalar = _mm_set1_epi32(scalar);
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&a[i]));
            __m128i vr = _mm_add_epi32(va, vScalar);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&result[i]), vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + scalar;
        }
#elif defined(MTYPE_SIMD_NEON)
        int32x4_t vScalar = vdupq_n_s32(scalar);
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            int32x4_t va = vld1q_s32(&a[i]);
            int32x4_t vr = vaddq_s32(va, vScalar);
            vst1q_s32(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + scalar;
        }
#else
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] + scalar;
        }
#endif
    }

    // ========== INTEGER SUBTRACTION ==========

    void SIMDOperations::subtractInt(const int* a, const int* b, int* result, size_t size)
    {
#if defined(MTYPE_SIMD_AVX2)
        size_t simdSize = size / 8 * 8;
        for (size_t i = 0; i < simdSize; i += 8) {
            __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&a[i]));
            __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&b[i]));
            __m256i vr = _mm256_sub_epi32(va, vb);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&result[i]), vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] - b[i];
        }
#elif defined(MTYPE_SIMD_SSE2)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&a[i]));
            __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&b[i]));
            __m128i vr = _mm_sub_epi32(va, vb);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&result[i]), vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] - b[i];
        }
#elif defined(MTYPE_SIMD_NEON)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            int32x4_t va = vld1q_s32(&a[i]);
            int32x4_t vb = vld1q_s32(&b[i]);
            int32x4_t vr = vsubq_s32(va, vb);
            vst1q_s32(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] - b[i];
        }
#else
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] - b[i];
        }
#endif
    }

    // ========== INTEGER MULTIPLICATION ==========

    void SIMDOperations::multiplyInt(const int* a, const int* b, int* result, size_t size)
    {
#if defined(MTYPE_SIMD_AVX2)
        size_t simdSize = size / 8 * 8;
        for (size_t i = 0; i < simdSize; i += 8) {
            __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&a[i]));
            __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&b[i]));
            __m256i vr = _mm256_mullo_epi32(va, vb);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&result[i]), vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] * b[i];
        }
#elif defined(MTYPE_SIMD_SSE2)
        // SSE2 doesn't have 32-bit integer multiply, need SSE4.1 (_mm_mullo_epi32)
        // For SSE2 compatibility, use scalar fallback or implement with SSE2 complex multiply
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] * b[i];
        }
#elif defined(MTYPE_SIMD_NEON)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            int32x4_t va = vld1q_s32(&a[i]);
            int32x4_t vb = vld1q_s32(&b[i]);
            int32x4_t vr = vmulq_s32(va, vb);
            vst1q_s32(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] * b[i];
        }
#else
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] * b[i];
        }
#endif
    }

    void SIMDOperations::multiplyScalarInt(const int* a, int scalar, int* result, size_t size)
    {
#if defined(MTYPE_SIMD_AVX2)
        __m256i vScalar = _mm256_set1_epi32(scalar);
        size_t simdSize = size / 8 * 8;
        for (size_t i = 0; i < simdSize; i += 8) {
            __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&a[i]));
            __m256i vr = _mm256_mullo_epi32(va, vScalar);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&result[i]), vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] * scalar;
        }
#elif defined(MTYPE_SIMD_NEON)
        int32x4_t vScalar = vdupq_n_s32(scalar);
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            int32x4_t va = vld1q_s32(&a[i]);
            int32x4_t vr = vmulq_s32(va, vScalar);
            vst1q_s32(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] * scalar;
        }
#else
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] * scalar;
        }
#endif
    }

    // ========== INTEGER REDUCTIONS ==========

    int SIMDOperations::sumInt(const int* data, size_t size)
    {
        int total = 0;

#if defined(MTYPE_SIMD_AVX2)
        __m256i vSum = _mm256_setzero_si256();
        size_t simdSize = size / 8 * 8;

        for (size_t i = 0; i < simdSize; i += 8) {
            __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&data[i]));
            vSum = _mm256_add_epi32(vSum, v);
        }

        // Horizontal sum: reduce 8 ints to 1
        __m128i vLow = _mm256_castsi256_si128(vSum);
        __m128i vHigh = _mm256_extracti128_si256(vSum, 1);
        __m128i vSum128 = _mm_add_epi32(vLow, vHigh);

        __m128i vShuf = _mm_shuffle_epi32(vSum128, _MM_SHUFFLE(1, 0, 3, 2));
        vSum128 = _mm_add_epi32(vSum128, vShuf);
        vShuf = _mm_shuffle_epi32(vSum128, _MM_SHUFFLE(2, 3, 0, 1));
        vSum128 = _mm_add_epi32(vSum128, vShuf);

        total = _mm_cvtsi128_si32(vSum128);

        for (size_t i = simdSize; i < size; ++i) {
            total += data[i];
        }
#elif defined(MTYPE_SIMD_SSE2)
        __m128i vSum = _mm_setzero_si128();
        size_t simdSize = size / 4 * 4;

        for (size_t i = 0; i < simdSize; i += 4) {
            __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&data[i]));
            vSum = _mm_add_epi32(vSum, v);
        }

        // Horizontal sum: reduce 4 ints to 1
        __m128i vShuf = _mm_shuffle_epi32(vSum, _MM_SHUFFLE(1, 0, 3, 2));
        vSum = _mm_add_epi32(vSum, vShuf);
        vShuf = _mm_shuffle_epi32(vSum, _MM_SHUFFLE(2, 3, 0, 1));
        vSum = _mm_add_epi32(vSum, vShuf);

        total = _mm_cvtsi128_si32(vSum);

        for (size_t i = simdSize; i < size; ++i) {
            total += data[i];
        }
#elif defined(MTYPE_SIMD_NEON)
        int32x4_t vSum = vdupq_n_s32(0);
        size_t simdSize = size / 4 * 4;

        for (size_t i = 0; i < simdSize; i += 4) {
            int32x4_t v = vld1q_s32(&data[i]);
            vSum = vaddq_s32(vSum, v);
        }

        // Horizontal sum
        total = vaddvq_s32(vSum);

        for (size_t i = simdSize; i < size; ++i) {
            total += data[i];
        }
#else
        for (size_t i = 0; i < size; ++i) {
            total += data[i];
        }
#endif

        return total;
    }

    int SIMDOperations::minInt(const int* data, size_t size)
    {
        if (size == 0) return std::numeric_limits<int>::max();

        int minVal = data[0];

#if defined(MTYPE_SIMD_AVX2)
        __m256i vMin = _mm256_set1_epi32(minVal);
        size_t simdSize = size / 8 * 8;

        for (size_t i = 0; i < simdSize; i += 8) {
            __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&data[i]));
            vMin = _mm256_min_epi32(vMin, v);
        }

        // Horizontal min
        __m128i vLow = _mm256_castsi256_si128(vMin);
        __m128i vHigh = _mm256_extracti128_si256(vMin, 1);
        __m128i vMin128 = _mm_min_epi32(vLow, vHigh);

        __m128i vShuf = _mm_shuffle_epi32(vMin128, _MM_SHUFFLE(1, 0, 3, 2));
        vMin128 = _mm_min_epi32(vMin128, vShuf);
        vShuf = _mm_shuffle_epi32(vMin128, _MM_SHUFFLE(2, 3, 0, 1));
        vMin128 = _mm_min_epi32(vMin128, vShuf);

        minVal = _mm_cvtsi128_si32(vMin128);

        for (size_t i = simdSize; i < size; ++i) {
            minVal = std::min(minVal, data[i]);
        }
#elif defined(MTYPE_SIMD_SSE2)
        // SSE2 doesn't have _mm_min_epi32, use SSE4.1 or scalar
        for (size_t i = 1; i < size; ++i) {
            minVal = std::min(minVal, data[i]);
        }
#elif defined(MTYPE_SIMD_NEON)
        int32x4_t vMin = vdupq_n_s32(minVal);
        size_t simdSize = size / 4 * 4;

        for (size_t i = 0; i < simdSize; i += 4) {
            int32x4_t v = vld1q_s32(&data[i]);
            vMin = vminq_s32(vMin, v);
        }

        // Horizontal min
        minVal = vminvq_s32(vMin);

        for (size_t i = simdSize; i < size; ++i) {
            minVal = std::min(minVal, data[i]);
        }
#else
        for (size_t i = 1; i < size; ++i) {
            minVal = std::min(minVal, data[i]);
        }
#endif

        return minVal;
    }

    int SIMDOperations::maxInt(const int* data, size_t size)
    {
        if (size == 0) return std::numeric_limits<int>::min();

        int maxVal = data[0];

#if defined(MTYPE_SIMD_AVX2)
        __m256i vMax = _mm256_set1_epi32(maxVal);
        size_t simdSize = size / 8 * 8;

        for (size_t i = 0; i < simdSize; i += 8) {
            __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&data[i]));
            vMax = _mm256_max_epi32(vMax, v);
        }

        // Horizontal max
        __m128i vLow = _mm256_castsi256_si128(vMax);
        __m128i vHigh = _mm256_extracti128_si256(vMax, 1);
        __m128i vMax128 = _mm_max_epi32(vLow, vHigh);

        __m128i vShuf = _mm_shuffle_epi32(vMax128, _MM_SHUFFLE(1, 0, 3, 2));
        vMax128 = _mm_max_epi32(vMax128, vShuf);
        vShuf = _mm_shuffle_epi32(vMax128, _MM_SHUFFLE(2, 3, 0, 1));
        vMax128 = _mm_max_epi32(vMax128, vShuf);

        maxVal = _mm_cvtsi128_si32(vMax128);

        for (size_t i = simdSize; i < size; ++i) {
            maxVal = std::max(maxVal, data[i]);
        }
#elif defined(MTYPE_SIMD_SSE2)
        // SSE2 doesn't have _mm_max_epi32, use SSE4.1 or scalar
        for (size_t i = 1; i < size; ++i) {
            maxVal = std::max(maxVal, data[i]);
        }
#elif defined(MTYPE_SIMD_NEON)
        int32x4_t vMax = vdupq_n_s32(maxVal);
        size_t simdSize = size / 4 * 4;

        for (size_t i = 0; i < simdSize; i += 4) {
            int32x4_t v = vld1q_s32(&data[i]);
            vMax = vmaxq_s32(vMax, v);
        }

        // Horizontal max
        maxVal = vmaxvq_s32(vMax);

        for (size_t i = simdSize; i < size; ++i) {
            maxVal = std::max(maxVal, data[i]);
        }
#else
        for (size_t i = 1; i < size; ++i) {
            maxVal = std::max(maxVal, data[i]);
        }
#endif

        return maxVal;
    }

    // ========== FLOAT OPERATIONS ==========
    // Similar implementation for floats using _mm_add_ps, _mm_sub_ps, _mm_mul_ps etc.

    void SIMDOperations::addFloat(const float* a, const float* b, float* result, size_t size)
    {
#if defined(MTYPE_SIMD_AVX2)
        size_t simdSize = size / 8 * 8;
        for (size_t i = 0; i < simdSize; i += 8) {
            __m256 va = _mm256_loadu_ps(&a[i]);
            __m256 vb = _mm256_loadu_ps(&b[i]);
            __m256 vr = _mm256_add_ps(va, vb);
            _mm256_storeu_ps(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + b[i];
        }
#elif defined(MTYPE_SIMD_SSE2)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            __m128 va = _mm_loadu_ps(&a[i]);
            __m128 vb = _mm_loadu_ps(&b[i]);
            __m128 vr = _mm_add_ps(va, vb);
            _mm_storeu_ps(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + b[i];
        }
#elif defined(MTYPE_SIMD_NEON)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vb = vld1q_f32(&b[i]);
            float32x4_t vr = vaddq_f32(va, vb);
            vst1q_f32(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + b[i];
        }
#else
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] + b[i];
        }
#endif
    }

    void SIMDOperations::addScalarFloat(const float* a, float scalar, float* result, size_t size)
    {
#if defined(MTYPE_SIMD_AVX2)
        __m256 vScalar = _mm256_set1_ps(scalar);
        size_t simdSize = size / 8 * 8;
        for (size_t i = 0; i < simdSize; i += 8) {
            __m256 va = _mm256_loadu_ps(&a[i]);
            __m256 vr = _mm256_add_ps(va, vScalar);
            _mm256_storeu_ps(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + scalar;
        }
#elif defined(MTYPE_SIMD_SSE2)
        __m128 vScalar = _mm_set1_ps(scalar);
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            __m128 va = _mm_loadu_ps(&a[i]);
            __m128 vr = _mm_add_ps(va, vScalar);
            _mm_storeu_ps(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + scalar;
        }
#elif defined(MTYPE_SIMD_NEON)
        float32x4_t vScalar = vdupq_n_f32(scalar);
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vr = vaddq_f32(va, vScalar);
            vst1q_f32(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] + scalar;
        }
#else
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] + scalar;
        }
#endif
    }

    void SIMDOperations::subtractFloat(const float* a, const float* b, float* result, size_t size)
    {
#if defined(MTYPE_SIMD_AVX2)
        size_t simdSize = size / 8 * 8;
        for (size_t i = 0; i < simdSize; i += 8) {
            __m256 va = _mm256_loadu_ps(&a[i]);
            __m256 vb = _mm256_loadu_ps(&b[i]);
            __m256 vr = _mm256_sub_ps(va, vb);
            _mm256_storeu_ps(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] - b[i];
        }
#elif defined(MTYPE_SIMD_SSE2)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            __m128 va = _mm_loadu_ps(&a[i]);
            __m128 vb = _mm_loadu_ps(&b[i]);
            __m128 vr = _mm_sub_ps(va, vb);
            _mm_storeu_ps(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] - b[i];
        }
#elif defined(MTYPE_SIMD_NEON)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vb = vld1q_f32(&b[i]);
            float32x4_t vr = vsubq_f32(va, vb);
            vst1q_f32(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] - b[i];
        }
#else
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] - b[i];
        }
#endif
    }

    void SIMDOperations::multiplyFloat(const float* a, const float* b, float* result, size_t size)
    {
#if defined(MTYPE_SIMD_AVX2)
        size_t simdSize = size / 8 * 8;
        for (size_t i = 0; i < simdSize; i += 8) {
            __m256 va = _mm256_loadu_ps(&a[i]);
            __m256 vb = _mm256_loadu_ps(&b[i]);
            __m256 vr = _mm256_mul_ps(va, vb);
            _mm256_storeu_ps(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] * b[i];
        }
#elif defined(MTYPE_SIMD_SSE2)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            __m128 va = _mm_loadu_ps(&a[i]);
            __m128 vb = _mm_loadu_ps(&b[i]);
            __m128 vr = _mm_mul_ps(va, vb);
            _mm_storeu_ps(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] * b[i];
        }
#elif defined(MTYPE_SIMD_NEON)
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vb = vld1q_f32(&b[i]);
            float32x4_t vr = vmulq_f32(va, vb);
            vst1q_f32(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] * b[i];
        }
#else
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] * b[i];
        }
#endif
    }

    void SIMDOperations::multiplyScalarFloat(const float* a, float scalar, float* result, size_t size)
    {
#if defined(MTYPE_SIMD_AVX2)
        __m256 vScalar = _mm256_set1_ps(scalar);
        size_t simdSize = size / 8 * 8;
        for (size_t i = 0; i < simdSize; i += 8) {
            __m256 va = _mm256_loadu_ps(&a[i]);
            __m256 vr = _mm256_mul_ps(va, vScalar);
            _mm256_storeu_ps(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] * scalar;
        }
#elif defined(MTYPE_SIMD_SSE2)
        __m128 vScalar = _mm_set1_ps(scalar);
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            __m128 va = _mm_loadu_ps(&a[i]);
            __m128 vr = _mm_mul_ps(va, vScalar);
            _mm_storeu_ps(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] * scalar;
        }
#elif defined(MTYPE_SIMD_NEON)
        float32x4_t vScalar = vdupq_n_f32(scalar);
        size_t simdSize = size / 4 * 4;
        for (size_t i = 0; i < simdSize; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vr = vmulq_f32(va, vScalar);
            vst1q_f32(&result[i], vr);
        }
        for (size_t i = simdSize; i < size; ++i) {
            result[i] = a[i] * scalar;
        }
#else
        for (size_t i = 0; i < size; ++i) {
            result[i] = a[i] * scalar;
        }
#endif
    }

    float SIMDOperations::sumFloat(const float* data, size_t size)
    {
        float total = 0.0f;

#if defined(MTYPE_SIMD_AVX2)
        __m256 vSum = _mm256_setzero_ps();
        size_t simdSize = size / 8 * 8;

        for (size_t i = 0; i < simdSize; i += 8) {
            __m256 v = _mm256_loadu_ps(&data[i]);
            vSum = _mm256_add_ps(vSum, v);
        }

        // Horizontal sum
        __m128 vLow = _mm256_castps256_ps128(vSum);
        __m128 vHigh = _mm256_extractf128_ps(vSum, 1);
        __m128 vSum128 = _mm_add_ps(vLow, vHigh);

        __m128 vShuf = _mm_shuffle_ps(vSum128, vSum128, _MM_SHUFFLE(1, 0, 3, 2));
        vSum128 = _mm_add_ps(vSum128, vShuf);
        vShuf = _mm_shuffle_ps(vSum128, vSum128, _MM_SHUFFLE(2, 3, 0, 1));
        vSum128 = _mm_add_ps(vSum128, vShuf);

        total = _mm_cvtss_f32(vSum128);

        for (size_t i = simdSize; i < size; ++i) {
            total += data[i];
        }
#elif defined(MTYPE_SIMD_SSE2)
        __m128 vSum = _mm_setzero_ps();
        size_t simdSize = size / 4 * 4;

        for (size_t i = 0; i < simdSize; i += 4) {
            __m128 v = _mm_loadu_ps(&data[i]);
            vSum = _mm_add_ps(vSum, v);
        }

        // Horizontal sum
        __m128 vShuf = _mm_shuffle_ps(vSum, vSum, _MM_SHUFFLE(1, 0, 3, 2));
        vSum = _mm_add_ps(vSum, vShuf);
        vShuf = _mm_shuffle_ps(vSum, vSum, _MM_SHUFFLE(2, 3, 0, 1));
        vSum = _mm_add_ps(vSum, vShuf);

        total = _mm_cvtss_f32(vSum);

        for (size_t i = simdSize; i < size; ++i) {
            total += data[i];
        }
#elif defined(MTYPE_SIMD_NEON)
        float32x4_t vSum = vdupq_n_f32(0.0f);
        size_t simdSize = size / 4 * 4;

        for (size_t i = 0; i < simdSize; i += 4) {
            float32x4_t v = vld1q_f32(&data[i]);
            vSum = vaddq_f32(vSum, v);
        }

        // Horizontal sum
        total = vaddvq_f32(vSum);

        for (size_t i = simdSize; i < size; ++i) {
            total += data[i];
        }
#else
        for (size_t i = 0; i < size; ++i) {
            total += data[i];
        }
#endif

        return total;
    }

    float SIMDOperations::minFloat(const float* data, size_t size)
    {
        if (size == 0) return std::numeric_limits<float>::max();

        float minVal = data[0];

#if defined(MTYPE_SIMD_AVX2)
        __m256 vMin = _mm256_set1_ps(minVal);
        size_t simdSize = size / 8 * 8;

        for (size_t i = 0; i < simdSize; i += 8) {
            __m256 v = _mm256_loadu_ps(&data[i]);
            vMin = _mm256_min_ps(vMin, v);
        }

        // Horizontal min
        __m128 vLow = _mm256_castps256_ps128(vMin);
        __m128 vHigh = _mm256_extractf128_ps(vMin, 1);
        __m128 vMin128 = _mm_min_ps(vLow, vHigh);

        __m128 vShuf = _mm_shuffle_ps(vMin128, vMin128, _MM_SHUFFLE(1, 0, 3, 2));
        vMin128 = _mm_min_ps(vMin128, vShuf);
        vShuf = _mm_shuffle_ps(vMin128, vMin128, _MM_SHUFFLE(2, 3, 0, 1));
        vMin128 = _mm_min_ps(vMin128, vShuf);

        minVal = _mm_cvtss_f32(vMin128);

        for (size_t i = simdSize; i < size; ++i) {
            minVal = std::min(minVal, data[i]);
        }
#elif defined(MTYPE_SIMD_SSE2)
        __m128 vMin = _mm_set1_ps(minVal);
        size_t simdSize = size / 4 * 4;

        for (size_t i = 0; i < simdSize; i += 4) {
            __m128 v = _mm_loadu_ps(&data[i]);
            vMin = _mm_min_ps(vMin, v);
        }

        // Horizontal min
        __m128 vShuf = _mm_shuffle_ps(vMin, vMin, _MM_SHUFFLE(1, 0, 3, 2));
        vMin = _mm_min_ps(vMin, vShuf);
        vShuf = _mm_shuffle_ps(vMin, vMin, _MM_SHUFFLE(2, 3, 0, 1));
        vMin = _mm_min_ps(vMin, vShuf);

        minVal = _mm_cvtss_f32(vMin);

        for (size_t i = simdSize; i < size; ++i) {
            minVal = std::min(minVal, data[i]);
        }
#elif defined(MTYPE_SIMD_NEON)
        float32x4_t vMin = vdupq_n_f32(minVal);
        size_t simdSize = size / 4 * 4;

        for (size_t i = 0; i < simdSize; i += 4) {
            float32x4_t v = vld1q_f32(&data[i]);
            vMin = vminq_f32(vMin, v);
        }

        // Horizontal min
        minVal = vminvq_f32(vMin);

        for (size_t i = simdSize; i < size; ++i) {
            minVal = std::min(minVal, data[i]);
        }
#else
        for (size_t i = 1; i < size; ++i) {
            minVal = std::min(minVal, data[i]);
        }
#endif

        return minVal;
    }

    float SIMDOperations::maxFloat(const float* data, size_t size)
    {
        if (size == 0) return std::numeric_limits<float>::min();

        float maxVal = data[0];

#if defined(MTYPE_SIMD_AVX2)
        __m256 vMax = _mm256_set1_ps(maxVal);
        size_t simdSize = size / 8 * 8;

        for (size_t i = 0; i < simdSize; i += 8) {
            __m256 v = _mm256_loadu_ps(&data[i]);
            vMax = _mm256_max_ps(vMax, v);
        }

        // Horizontal max
        __m128 vLow = _mm256_castps256_ps128(vMax);
        __m128 vHigh = _mm256_extractf128_ps(vMax, 1);
        __m128 vMax128 = _mm_max_ps(vLow, vHigh);

        __m128 vShuf = _mm_shuffle_ps(vMax128, vMax128, _MM_SHUFFLE(1, 0, 3, 2));
        vMax128 = _mm_max_ps(vMax128, vShuf);
        vShuf = _mm_shuffle_ps(vMax128, vMax128, _MM_SHUFFLE(2, 3, 0, 1));
        vMax128 = _mm_max_ps(vMax128, vShuf);

        maxVal = _mm_cvtss_f32(vMax128);

        for (size_t i = simdSize; i < size; ++i) {
            maxVal = std::max(maxVal, data[i]);
        }
#elif defined(MTYPE_SIMD_SSE2)
        __m128 vMax = _mm_set1_ps(maxVal);
        size_t simdSize = size / 4 * 4;

        for (size_t i = 0; i < simdSize; i += 4) {
            __m128 v = _mm_loadu_ps(&data[i]);
            vMax = _mm_max_ps(vMax, v);
        }

        // Horizontal max
        __m128 vShuf = _mm_shuffle_ps(vMax, vMax, _MM_SHUFFLE(1, 0, 3, 2));
        vMax = _mm_max_ps(vMax, vShuf);
        vShuf = _mm_shuffle_ps(vMax, vMax, _MM_SHUFFLE(2, 3, 0, 1));
        vMax = _mm_max_ps(vMax, vShuf);

        maxVal = _mm_cvtss_f32(vMax);

        for (size_t i = simdSize; i < size; ++i) {
            maxVal = std::max(maxVal, data[i]);
        }
#elif defined(MTYPE_SIMD_NEON)
        float32x4_t vMax = vdupq_n_f32(maxVal);
        size_t simdSize = size / 4 * 4;

        for (size_t i = 0; i < simdSize; i += 4) {
            float32x4_t v = vld1q_f32(&data[i]);
            vMax = vmaxq_f32(vMax, v);
        }

        // Horizontal max
        maxVal = vmaxvq_f32(vMax);

        for (size_t i = simdSize; i < size; ++i) {
            maxVal = std::max(maxVal, data[i]);
        }
#else
        for (size_t i = 1; i < size; ++i) {
            maxVal = std::max(maxVal, data[i]);
        }
#endif

        return maxVal;
    }

    // ========== UTILITY OPERATIONS ==========
    // Simplified implementations for utility functions

    void SIMDOperations::fillInt(int* data, int value, size_t size)
    {
        std::fill_n(data, size, value);
    }

    void SIMDOperations::fillFloat(float* data, float value, size_t size)
    {
        std::fill_n(data, size, value);
    }

    void SIMDOperations::copyInt(const int* src, int* dest, size_t size)
    {
        std::copy_n(src, size, dest);
    }

    void SIMDOperations::copyFloat(const float* src, float* dest, size_t size)
    {
        std::copy_n(src, size, dest);
    }

    void SIMDOperations::reverseInt(int* data, size_t size)
    {
        std::reverse(data, data + size);
    }

    void SIMDOperations::reverseFloat(float* data, size_t size)
    {
        std::reverse(data, data + size);
    }

} // namespace mType::value::simd
