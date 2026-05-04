#pragma once

#include "SIMDConfig.hpp"
#include <cstdint>
#include <cstddef>

namespace mType::value::simd
{
    /**
     * @brief SIMD Policy Architecture for eliminating code duplication
     *
     * Design Pattern: Policy-based design (template metaprogramming)
     *
     * Each policy encapsulates:
     * - Vector width (how many elements per SIMD register)
     * - Load/store operations (aligned vs unaligned)
     * - Arithmetic operations (add, sub, mul, etc.)
     * - Reduction operations (horizontal sum/min/max)
     *
     * Benefits:
     * - DRY: Single implementation for each operation type
     * - Type Safety: Compile-time dispatch, zero runtime overhead
     * - Maintainability: Add new SIMD instruction sets by adding new policy
     * - Testability: Easy to test each policy independently
     */

    // ========== INTEGER SIMD POLICIES ==========

#if defined(MTYPE_SIMD_AVX2)
    /**
     * @brief AVX2 policy for 32-bit integers (256-bit vectors, 8 elements)
     */
    struct AVX2IntPolicy
    {
        using VectorType = __m256i;
        static constexpr size_t WIDTH = 8;
        static constexpr const char* NAME = "AVX2";

        // Load/Store
        static inline VectorType load(const int* ptr) { return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr)); }
        static inline void store(int* ptr, VectorType v) { _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), v); }

        // Arithmetic
        static inline VectorType add(VectorType a, VectorType b) { return _mm256_add_epi32(a, b); }
        static inline VectorType subtract(VectorType a, VectorType b) { return _mm256_sub_epi32(a, b); }
        static inline VectorType multiply(VectorType a, VectorType b) { return _mm256_mullo_epi32(a, b); }
        static inline VectorType set1(int value) { return _mm256_set1_epi32(value); }

        // Reductions
        static inline VectorType min(VectorType a, VectorType b) { return _mm256_min_epi32(a, b); }
        static inline VectorType max(VectorType a, VectorType b) { return _mm256_max_epi32(a, b); }
        static inline VectorType zero() { return _mm256_setzero_si256(); }

        // Horizontal reduction: reduce vector to single value
        static inline int horizontal_sum(VectorType v)
        {
            __m128i vLow = _mm256_castsi256_si128(v);
            __m128i vHigh = _mm256_extracti128_si256(v, 1);
            __m128i vSum = _mm_add_epi32(vLow, vHigh);
            __m128i vShuf = _mm_shuffle_epi32(vSum, _MM_SHUFFLE(1, 0, 3, 2));
            vSum = _mm_add_epi32(vSum, vShuf);
            vShuf = _mm_shuffle_epi32(vSum, _MM_SHUFFLE(2, 3, 0, 1));
            vSum = _mm_add_epi32(vSum, vShuf);
            return _mm_cvtsi128_si32(vSum);
        }

        static inline int horizontal_min(VectorType v)
        {
            __m128i vLow = _mm256_castsi256_si128(v);
            __m128i vHigh = _mm256_extracti128_si256(v, 1);
            __m128i vMin = _mm_min_epi32(vLow, vHigh);
            __m128i vShuf = _mm_shuffle_epi32(vMin, _MM_SHUFFLE(1, 0, 3, 2));
            vMin = _mm_min_epi32(vMin, vShuf);
            vShuf = _mm_shuffle_epi32(vMin, _MM_SHUFFLE(2, 3, 0, 1));
            vMin = _mm_min_epi32(vMin, vShuf);
            return _mm_cvtsi128_si32(vMin);
        }

        static inline int horizontal_max(VectorType v)
        {
            __m128i vLow = _mm256_castsi256_si128(v);
            __m128i vHigh = _mm256_extracti128_si256(v, 1);
            __m128i vMax = _mm_max_epi32(vLow, vHigh);
            __m128i vShuf = _mm_shuffle_epi32(vMax, _MM_SHUFFLE(1, 0, 3, 2));
            vMax = _mm_max_epi32(vMax, vShuf);
            vShuf = _mm_shuffle_epi32(vMax, _MM_SHUFFLE(2, 3, 0, 1));
            vMax = _mm_max_epi32(vMax, vShuf);
            return _mm_cvtsi128_si32(vMax);
        }
    };

    using IntPolicy = AVX2IntPolicy;

#elif defined(MTYPE_SIMD_SSE2)
    /**
     * @brief SSE2/SSE4.1 policy for 32-bit integers (128-bit vectors, 4 elements)
     *
     * Note: Pure SSE2 lacks 32-bit multiply and min/max operations.
     * This implementation uses SSE4.1 intrinsics (_mm_mullo_epi32, _mm_min_epi32, _mm_max_epi32)
     * which are available via the build config (-msse4.1).
     *
     * Fallback: If SSE4.1 is unavailable, SIMDOperationsImpl uses scalar fallback via compile-time checks.
     */
    struct SSE2IntPolicy
    {
        using VectorType = __m128i;
        static constexpr size_t WIDTH = 4;
        static constexpr const char* NAME = "SSE2/SSE4.1";

        // Load/Store
        static inline VectorType load(const int* ptr) { return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr)); }
        static inline void store(int* ptr, VectorType v) { _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), v); }

        // Arithmetic
        static inline VectorType add(VectorType a, VectorType b) { return _mm_add_epi32(a, b); }
        static inline VectorType subtract(VectorType a, VectorType b) { return _mm_sub_epi32(a, b); }
        static inline VectorType multiply(VectorType a, VectorType b)
        {
#ifdef __SSE4_1__
            // SSE4.1: Use 32-bit multiply intrinsic
            return _mm_mullo_epi32(a, b);
#else
            // SSE2 fallback: Emulate using 16-bit multiplies (slower but correct)
            // This path should rarely execute as build config includes -msse4.1
            __m128i a_lo = _mm_shuffle_epi32(a, 0xA0);  // a0, a0, a2, a2
            __m128i a_hi = _mm_shuffle_epi32(a, 0xF5);  // a1, a1, a3, a3
            __m128i b_lo = _mm_shuffle_epi32(b, 0xA0);
            __m128i b_hi = _mm_shuffle_epi32(b, 0xF5);
            __m128i mul_lo = _mm_mul_epu32(a_lo, b_lo);
            __m128i mul_hi = _mm_mul_epu32(a_hi, b_hi);
            return _mm_unpacklo_epi32(_mm_shuffle_epi32(mul_lo, 0x08), _mm_shuffle_epi32(mul_hi, 0x08));
#endif
        }
        static inline VectorType set1(int value) { return _mm_set1_epi32(value); }

        // Reductions
        static inline VectorType min(VectorType a, VectorType b)
        {
#ifdef __SSE4_1__
            return _mm_min_epi32(a, b);
#else
            // SSE2 fallback: Compare and blend manually
            __m128i mask = _mm_cmplt_epi32(a, b);  // a < b ? 0xFFFFFFFF : 0
            return _mm_or_si128(_mm_and_si128(mask, a), _mm_andnot_si128(mask, b));
#endif
        }

        static inline VectorType max(VectorType a, VectorType b)
        {
#ifdef __SSE4_1__
            return _mm_max_epi32(a, b);
#else
            // SSE2 fallback: Compare and blend manually
            __m128i mask = _mm_cmpgt_epi32(a, b);  // a > b ? 0xFFFFFFFF : 0
            return _mm_or_si128(_mm_and_si128(mask, a), _mm_andnot_si128(mask, b));
#endif
        }

        static inline VectorType zero() { return _mm_setzero_si128(); }

        static inline int horizontal_sum(VectorType v)
        {
            __m128i vShuf = _mm_shuffle_epi32(v, _MM_SHUFFLE(1, 0, 3, 2));
            v = _mm_add_epi32(v, vShuf);
            vShuf = _mm_shuffle_epi32(v, _MM_SHUFFLE(2, 3, 0, 1));
            v = _mm_add_epi32(v, vShuf);
            return _mm_cvtsi128_si32(v);
        }

        static inline int horizontal_min(VectorType v)
        {
            // Horizontal minimum using min operation
            __m128i vShuf = _mm_shuffle_epi32(v, _MM_SHUFFLE(1, 0, 3, 2));
            v = min(v, vShuf);
            vShuf = _mm_shuffle_epi32(v, _MM_SHUFFLE(2, 3, 0, 1));
            v = min(v, vShuf);
            return _mm_cvtsi128_si32(v);
        }

        static inline int horizontal_max(VectorType v)
        {
            // Horizontal maximum using max operation
            __m128i vShuf = _mm_shuffle_epi32(v, _MM_SHUFFLE(1, 0, 3, 2));
            v = max(v, vShuf);
            vShuf = _mm_shuffle_epi32(v, _MM_SHUFFLE(2, 3, 0, 1));
            v = max(v, vShuf);
            return _mm_cvtsi128_si32(v);
        }
    };

    using IntPolicy = SSE2IntPolicy;

#elif defined(MTYPE_SIMD_NEON)
    /**
     * @brief NEON policy for 32-bit integers (128-bit vectors, 4 elements)
     */
    struct NEONIntPolicy
    {
        using VectorType = int32x4_t;
        static constexpr size_t WIDTH = 4;
        static constexpr const char* NAME = "NEON";

        // Load/Store
        static inline VectorType load(const int* ptr) { return vld1q_s32(ptr); }
        static inline void store(int* ptr, VectorType v) { vst1q_s32(ptr, v); }

        // Arithmetic
        static inline VectorType add(VectorType a, VectorType b) { return vaddq_s32(a, b); }
        static inline VectorType subtract(VectorType a, VectorType b) { return vsubq_s32(a, b); }
        static inline VectorType multiply(VectorType a, VectorType b) { return vmulq_s32(a, b); }
        static inline VectorType set1(int value) { return vdupq_n_s32(value); }

        // Reductions
        static inline VectorType min(VectorType a, VectorType b) { return vminq_s32(a, b); }
        static inline VectorType max(VectorType a, VectorType b) { return vmaxq_s32(a, b); }
        static inline VectorType zero() { return vdupq_n_s32(0); }

        // Horizontal reductions
        static inline int horizontal_sum(VectorType v) { return vaddvq_s32(v); }
        static inline int horizontal_min(VectorType v) { return vminvq_s32(v); }
        static inline int horizontal_max(VectorType v) { return vmaxvq_s32(v); }
    };

    using IntPolicy = NEONIntPolicy;

#else
    /**
     * @brief Scalar fallback policy (no SIMD)
     */
    struct ScalarIntPolicy
    {
        using VectorType = int;
        static constexpr size_t WIDTH = 1;
        static constexpr const char* NAME = "Scalar";

        static inline VectorType load(const int* ptr) { return *ptr; }
        static inline void store(int* ptr, VectorType v) { *ptr = v; }

        static inline VectorType add(VectorType a, VectorType b) { return a + b; }
        static inline VectorType subtract(VectorType a, VectorType b) { return a - b; }
        static inline VectorType multiply(VectorType a, VectorType b) { return a * b; }
        static inline VectorType set1(int value) { return value; }

        static inline VectorType min(VectorType a, VectorType b) { return std::min(a, b); }
        static inline VectorType max(VectorType a, VectorType b) { return std::max(a, b); }
        static inline VectorType zero() { return 0; }

        static inline int horizontal_sum(VectorType v) { return v; }
        static inline int horizontal_min(VectorType v) { return v; }
        static inline int horizontal_max(VectorType v) { return v; }
    };

    using IntPolicy = ScalarIntPolicy;
#endif

    // ========== INT64 POLICY (Scalar only - SIMD for 64-bit integers is complex) ==========
    /**
     * @brief Scalar policy for 64-bit integers
     *
     * Note: SIMD support for 64-bit integers is limited:
     * - SSE2: Has add/sub but NO multiply, NO min/max for 64-bit integers
     * - AVX2: Has add/sub but NO multiply, NO min/max for 64-bit integers
     * - AVX-512: Full support but limited availability
     *
     * For simplicity and portability, we use scalar operations for int64_t.
     * The performance impact is minimal since:
     * 1. Modern CPUs have fast 64-bit arithmetic
     * 2. Memory bandwidth is often the bottleneck anyway
     * 3. Most mType arrays are not large enough for SIMD to matter significantly
     */
    struct ScalarInt64Policy
    {
        using VectorType = int64_t;
        static constexpr size_t WIDTH = 1;
        static constexpr const char* NAME = "Scalar64";

        static inline VectorType load(const int64_t* ptr) { return *ptr; }
        static inline void store(int64_t* ptr, VectorType v) { *ptr = v; }

        static inline VectorType add(VectorType a, VectorType b) { return a + b; }
        static inline VectorType subtract(VectorType a, VectorType b) { return a - b; }
        static inline VectorType multiply(VectorType a, VectorType b) { return a * b; }
        static inline VectorType set1(int64_t value) { return value; }

        static inline VectorType min(VectorType a, VectorType b) { return std::min(a, b); }
        static inline VectorType max(VectorType a, VectorType b) { return std::max(a, b); }
        static inline VectorType zero() { return 0; }

        static inline int64_t horizontal_sum(VectorType v) { return v; }
        static inline int64_t horizontal_min(VectorType v) { return v; }
        static inline int64_t horizontal_max(VectorType v) { return v; }
    };

    using Int64Policy = ScalarInt64Policy;

    // ========== FLOAT SIMD POLICIES ==========

#if defined(MTYPE_SIMD_AVX2)
    /**
     * @brief AVX2 policy for 64-bit doubles (256-bit vectors, 4 elements)
     */
    struct AVX2FloatPolicy
    {
        using VectorType = __m256d;
        static constexpr size_t WIDTH = 4;
        static constexpr const char* NAME = "AVX2";

        // Load/Store
        static inline VectorType load(const double* ptr) { return _mm256_loadu_pd(ptr); }
        static inline void store(double* ptr, VectorType v) { _mm256_storeu_pd(ptr, v); }

        // Arithmetic
        static inline VectorType add(VectorType a, VectorType b) { return _mm256_add_pd(a, b); }
        static inline VectorType subtract(VectorType a, VectorType b) { return _mm256_sub_pd(a, b); }
        static inline VectorType multiply(VectorType a, VectorType b) { return _mm256_mul_pd(a, b); }
        static inline VectorType set1(double value) { return _mm256_set1_pd(value); }

        // Reductions
        static inline VectorType min(VectorType a, VectorType b) { return _mm256_min_pd(a, b); }
        static inline VectorType max(VectorType a, VectorType b) { return _mm256_max_pd(a, b); }
        static inline VectorType zero() { return _mm256_setzero_pd(); }

        // Horizontal reductions
        static inline double horizontal_sum(VectorType v)
        {
            __m128d vLow = _mm256_castpd256_pd128(v);
            __m128d vHigh = _mm256_extractf128_pd(v, 1);
            __m128d vSum = _mm_add_pd(vLow, vHigh);
            __m128d vShuf = _mm_shuffle_pd(vSum, vSum, 1);
            vSum = _mm_add_pd(vSum, vShuf);
            return _mm_cvtsd_f64(vSum);
        }

        static inline double horizontal_min(VectorType v)
        {
            __m128d vLow = _mm256_castpd256_pd128(v);
            __m128d vHigh = _mm256_extractf128_pd(v, 1);
            __m128d vMin = _mm_min_pd(vLow, vHigh);
            __m128d vShuf = _mm_shuffle_pd(vMin, vMin, 1);
            vMin = _mm_min_pd(vMin, vShuf);
            return _mm_cvtsd_f64(vMin);
        }

        static inline double horizontal_max(VectorType v)
        {
            __m128d vLow = _mm256_castpd256_pd128(v);
            __m128d vHigh = _mm256_extractf128_pd(v, 1);
            __m128d vMax = _mm_max_pd(vLow, vHigh);
            __m128d vShuf = _mm_shuffle_pd(vMax, vMax, 1);
            vMax = _mm_max_pd(vMax, vShuf);
            return _mm_cvtsd_f64(vMax);
        }
    };

    using FloatPolicy = AVX2FloatPolicy;

#elif defined(MTYPE_SIMD_SSE2)
    /**
     * @brief SSE2 policy for 64-bit doubles (128-bit vectors, 2 elements)
     */
    struct SSE2FloatPolicy
    {
        using VectorType = __m128d;
        static constexpr size_t WIDTH = 2;
        static constexpr const char* NAME = "SSE2";

        // Load/Store
        static inline VectorType load(const double* ptr) { return _mm_loadu_pd(ptr); }
        static inline void store(double* ptr, VectorType v) { _mm_storeu_pd(ptr, v); }

        // Arithmetic
        static inline VectorType add(VectorType a, VectorType b) { return _mm_add_pd(a, b); }
        static inline VectorType subtract(VectorType a, VectorType b) { return _mm_sub_pd(a, b); }
        static inline VectorType multiply(VectorType a, VectorType b) { return _mm_mul_pd(a, b); }
        static inline VectorType set1(double value) { return _mm_set1_pd(value); }

        // Reductions
        static inline VectorType min(VectorType a, VectorType b) { return _mm_min_pd(a, b); }
        static inline VectorType max(VectorType a, VectorType b) { return _mm_max_pd(a, b); }
        static inline VectorType zero() { return _mm_setzero_pd(); }

        // Horizontal reductions
        static inline double horizontal_sum(VectorType v)
        {
            __m128d vShuf = _mm_shuffle_pd(v, v, 1);
            v = _mm_add_pd(v, vShuf);
            return _mm_cvtsd_f64(v);
        }

        static inline double horizontal_min(VectorType v)
        {
            __m128d vShuf = _mm_shuffle_pd(v, v, 1);
            v = _mm_min_pd(v, vShuf);
            return _mm_cvtsd_f64(v);
        }

        static inline double horizontal_max(VectorType v)
        {
            __m128d vShuf = _mm_shuffle_pd(v, v, 1);
            v = _mm_max_pd(v, vShuf);
            return _mm_cvtsd_f64(v);
        }
    };

    using FloatPolicy = SSE2FloatPolicy;

#elif defined(MTYPE_SIMD_NEON)
    /**
     * @brief NEON policy for 64-bit doubles (128-bit vectors, 2 elements)
     */
    struct NEONFloatPolicy
    {
        using VectorType = float64x2_t;
        static constexpr size_t WIDTH = 2;
        static constexpr const char* NAME = "NEON";

        // Load/Store
        static inline VectorType load(const double* ptr) { return vld1q_f64(ptr); }
        static inline void store(double* ptr, VectorType v) { vst1q_f64(ptr, v); }

        // Arithmetic
        static inline VectorType add(VectorType a, VectorType b) { return vaddq_f64(a, b); }
        static inline VectorType subtract(VectorType a, VectorType b) { return vsubq_f64(a, b); }
        static inline VectorType multiply(VectorType a, VectorType b) { return vmulq_f64(a, b); }
        static inline VectorType set1(double value) { return vdupq_n_f64(value); }

        // Reductions
        static inline VectorType min(VectorType a, VectorType b) { return vminq_f64(a, b); }
        static inline VectorType max(VectorType a, VectorType b) { return vmaxq_f64(a, b); }
        static inline VectorType zero() { return vdupq_n_f64(0.0); }

        // Horizontal reductions
        static inline double horizontal_sum(VectorType v) { return vaddvq_f64(v); }
        static inline double horizontal_min(VectorType v) { return vminvq_f64(v); }
        static inline double horizontal_max(VectorType v) { return vmaxvq_f64(v); }
    };

    using FloatPolicy = NEONFloatPolicy;

#else
    /**
     * @brief Scalar fallback policy for doubles (no SIMD)
     */
    struct ScalarFloatPolicy
    {
        using VectorType = double;
        static constexpr size_t WIDTH = 1;
        static constexpr const char* NAME = "Scalar";

        static inline VectorType load(const double* ptr) { return *ptr; }
        static inline void store(double* ptr, VectorType v) { *ptr = v; }

        static inline VectorType add(VectorType a, VectorType b) { return a + b; }
        static inline VectorType subtract(VectorType a, VectorType b) { return a - b; }
        static inline VectorType multiply(VectorType a, VectorType b) { return a * b; }
        static inline VectorType set1(double value) { return value; }

        static inline VectorType min(VectorType a, VectorType b) { return std::min(a, b); }
        static inline VectorType max(VectorType a, VectorType b) { return std::max(a, b); }
        static inline VectorType zero() { return 0.0; }

        static inline double horizontal_sum(VectorType v) { return v; }
        static inline double horizontal_min(VectorType v) { return v; }
        static inline double horizontal_max(VectorType v) { return v; }
    };

    using FloatPolicy = ScalarFloatPolicy;
#endif

} // namespace mType::value::simd
