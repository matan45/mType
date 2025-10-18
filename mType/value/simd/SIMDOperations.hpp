#pragma once

#include "SIMDConfig.hpp"
#include <cstddef>
#include <algorithm>

#ifdef MTYPE_SIMD_ENABLED
    #if defined(MTYPE_SIMD_SSE2) || defined(MTYPE_SIMD_AVX2)
        #include <immintrin.h>
    #elif defined(MTYPE_SIMD_NEON)
        #include <arm_neon.h>
    #endif
#endif

namespace mType::value::simd
{
    /**
     * @brief High-performance SIMD operations for primitive arrays
     *
     * Provides vectorized implementations using:
     * - SSE2 (x86/x64): 128-bit vectors (4 ints or 4 floats at once)
     * - AVX2 (x86/x64): 256-bit vectors (8 ints or 8 floats at once)
     * - NEON (ARM): 128-bit vectors (4 ints or 4 floats at once)
     *
     * Performance gains:
     * - Arithmetic ops: 3-4× faster for arrays ≥16 elements
     * - Reduction ops: 4-8× faster for arrays ≥16 elements
     */
    class SIMDOperations
    {
    public:
        // ========== INTEGER OPERATIONS ==========

        /**
         * @brief SIMD-accelerated integer array addition: result[i] = a[i] + b[i]
         * @param a First array
         * @param b Second array
         * @param result Output array
         * @param size Number of elements (must be same for all arrays)
         */
        static void addInt(const int* a, const int* b, int* result, size_t size);

        /**
         * @brief SIMD-accelerated integer scalar addition: result[i] = a[i] + scalar
         */
        static void addScalarInt(const int* a, int scalar, int* result, size_t size);

        /**
         * @brief SIMD-accelerated integer array subtraction: result[i] = a[i] - b[i]
         */
        static void subtractInt(const int* a, const int* b, int* result, size_t size);

        /**
         * @brief SIMD-accelerated integer array multiplication: result[i] = a[i] * b[i]
         */
        static void multiplyInt(const int* a, const int* b, int* result, size_t size);

        /**
         * @brief SIMD-accelerated integer scalar multiplication: result[i] = a[i] * scalar
         */
        static void multiplyScalarInt(const int* a, int scalar, int* result, size_t size);

        /**
         * @brief SIMD-accelerated integer sum reduction
         * @return Sum of all elements
         */
        static int sumInt(const int* data, size_t size);

        /**
         * @brief SIMD-accelerated integer minimum
         * @return Minimum element
         */
        static int minInt(const int* data, size_t size);

        /**
         * @brief SIMD-accelerated integer maximum
         * @return Maximum element
         */
        static int maxInt(const int* data, size_t size);

        // ========== FLOAT OPERATIONS ==========

        /**
         * @brief SIMD-accelerated float array addition: result[i] = a[i] + b[i]
         */
        static void addFloat(const float* a, const float* b, float* result, size_t size);

        /**
         * @brief SIMD-accelerated float scalar addition: result[i] = a[i] + scalar
         */
        static void addScalarFloat(const float* a, float scalar, float* result, size_t size);

        /**
         * @brief SIMD-accelerated float array subtraction: result[i] = a[i] - b[i]
         */
        static void subtractFloat(const float* a, const float* b, float* result, size_t size);

        /**
         * @brief SIMD-accelerated float array multiplication: result[i] = a[i] * b[i]
         */
        static void multiplyFloat(const float* a, const float* b, float* result, size_t size);

        /**
         * @brief SIMD-accelerated float scalar multiplication: result[i] = a[i] * scalar
         */
        static void multiplyScalarFloat(const float* a, float scalar, float* result, size_t size);

        /**
         * @brief SIMD-accelerated float sum reduction
         * @return Sum of all elements
         */
        static float sumFloat(const float* data, size_t size);

        /**
         * @brief SIMD-accelerated float minimum
         * @return Minimum element
         */
        static float minFloat(const float* data, size_t size);

        /**
         * @brief SIMD-accelerated float maximum
         * @return Maximum element
         */
        static float maxFloat(const float* data, size_t size);

        // ========== UTILITY OPERATIONS ==========

        /**
         * @brief SIMD-accelerated fill: data[i] = value for all i
         */
        static void fillInt(int* data, int value, size_t size);
        static void fillFloat(float* data, float value, size_t size);

        /**
         * @brief SIMD-accelerated copy: dest[i] = src[i] for all i
         */
        static void copyInt(const int* src, int* dest, size_t size);
        static void copyFloat(const float* src, float* dest, size_t size);

        /**
         * @brief SIMD-accelerated reverse: data[i] <-> data[size-1-i]
         */
        static void reverseInt(int* data, size_t size);
        static void reverseFloat(float* data, size_t size);
    };

} // namespace mType::value::simd
