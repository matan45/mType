#pragma once

#include "SIMDPolicies.hpp"
#include <algorithm>
#include <limits>

namespace mType::value::simd
{
    // Forward declarations for compile-time checks
    struct SSE2IntPolicy;
    struct SSE2FloatPolicy;

    /**
     * @brief Template-based SIMD operations using policy-based design
     *
     * Benefits of this approach:
     * - DRY: Each operation written ONCE, not 4 times (AVX2/SSE2/NEON/Scalar)
     * - Type Safety: Compile-time dispatch based on available SIMD instruction set
     * - Zero Overhead: All policy methods are inline, no runtime cost
     * - Maintainability: To add new SIMD ISA, just add new policy (e.g., AVX512Policy)
     * - Testability: Can test with different policies independently
     *
     * Design Pattern: Policy-Based Design + Template Method Pattern
     */

    /**
     * @brief Generic binary operation template (add, subtract, multiply)
     * @tparam Policy SIMD policy (IntPolicy or FloatPolicy)
     * @tparam T Scalar type (int or float)
     */
    template<typename Policy, typename T>
    void binaryOperation(
        const T* a,
        const T* b,
        T* result,
        size_t size,
        typename Policy::VectorType (*vectorOp)(typename Policy::VectorType, typename Policy::VectorType))
    {
        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        // SIMD path: Process WIDTH elements at a time
        for (size_t i = 0; i < simdSize; i += WIDTH)
        {
            auto va = Policy::load(&a[i]);
            auto vb = Policy::load(&b[i]);
            auto vr = vectorOp(va, vb);
            Policy::store(&result[i], vr);
        }

        // Scalar remainder: Process remaining elements
        for (size_t i = simdSize; i < size; ++i)
        {
            result[i] = a[i] + b[i]; // This will be specialized per operation
        }
    }

    /**
     * @brief Generic scalar operation template (add scalar, multiply scalar)
     * @tparam Policy SIMD policy (IntPolicy or FloatPolicy)
     * @tparam T Scalar type (int or float)
     */
    template<typename Policy, typename T>
    void scalarOperation(
        const T* a,
        T scalar,
        T* result,
        size_t size,
        typename Policy::VectorType (*vectorOp)(typename Policy::VectorType, typename Policy::VectorType))
    {
        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        auto vScalar = Policy::set1(scalar);

        // SIMD path
        for (size_t i = 0; i < simdSize; i += WIDTH)
        {
            auto va = Policy::load(&a[i]);
            auto vr = vectorOp(va, vScalar);
            Policy::store(&result[i], vr);
        }

        // Scalar remainder - will be specialized
        for (size_t i = simdSize; i < size; ++i)
        {
            result[i] = a[i] + scalar; // Placeholder, will be specialized
        }
    }

    /**
     * @brief Generic reduction operation template (sum, min, max)
     * @tparam Policy SIMD policy (IntPolicy or FloatPolicy)
     * @tparam T Scalar type (int or float)
     */
    template<typename Policy, typename T>
    T reductionOperation(
        const T* data,
        size_t size,
        T initialValue,
        typename Policy::VectorType (*vectorOp)(typename Policy::VectorType, typename Policy::VectorType),
        T (*horizontalOp)(typename Policy::VectorType),
        T (*scalarOp)(T, T))
    {
        if (size == 0) return initialValue;

        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        // Initialize accumulator
        auto vAcc = Policy::set1(initialValue);

        // SIMD path: Accumulate WIDTH elements at a time
        for (size_t i = 0; i < simdSize; i += WIDTH)
        {
            auto v = Policy::load(&data[i]);
            vAcc = vectorOp(vAcc, v);
        }

        // Horizontal reduction: Collapse vector to single value
        T result = horizontalOp(vAcc);

        // Scalar remainder: Process remaining elements
        for (size_t i = simdSize; i < size; ++i)
        {
            result = scalarOp(result, data[i]);
        }

        return result;
    }

    // ========== SPECIALIZED IMPLEMENTATIONS ==========

    /**
     * @brief Add two arrays: result[i] = a[i] + b[i]
     */
    template<typename Policy, typename T>
    void addImpl(const T* a, const T* b, T* result, size_t size)
    {
        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        for (size_t i = 0; i < simdSize; i += WIDTH)
        {
            auto va = Policy::load(&a[i]);
            auto vb = Policy::load(&b[i]);
            auto vr = Policy::add(va, vb);
            Policy::store(&result[i], vr);
        }

        for (size_t i = simdSize; i < size; ++i)
        {
            result[i] = a[i] + b[i];
        }
    }

    /**
     * @brief Add scalar to array: result[i] = a[i] + scalar
     */
    template<typename Policy, typename T>
    void addScalarImpl(const T* a, T scalar, T* result, size_t size)
    {
        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        auto vScalar = Policy::set1(scalar);

        for (size_t i = 0; i < simdSize; i += WIDTH)
        {
            auto va = Policy::load(&a[i]);
            auto vr = Policy::add(va, vScalar);
            Policy::store(&result[i], vr);
        }

        for (size_t i = simdSize; i < size; ++i)
        {
            result[i] = a[i] + scalar;
        }
    }

    /**
     * @brief Subtract two arrays: result[i] = a[i] - b[i]
     */
    template<typename Policy, typename T>
    void subtractImpl(const T* a, const T* b, T* result, size_t size)
    {
        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        for (size_t i = 0; i < simdSize; i += WIDTH)
        {
            auto va = Policy::load(&a[i]);
            auto vb = Policy::load(&b[i]);
            auto vr = Policy::subtract(va, vb);
            Policy::store(&result[i], vr);
        }

        for (size_t i = simdSize; i < size; ++i)
        {
            result[i] = a[i] - b[i];
        }
    }

    /**
     * @brief Multiply two arrays: result[i] = a[i] * b[i]
     */
    template<typename Policy, typename T>
    void multiplyImpl(const T* a, const T* b, T* result, size_t size)
    {
        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        // Check if multiply is supported (SSE2 doesn't support 32-bit int multiply)
        if constexpr (WIDTH == 1 || std::is_same_v<Policy, SSE2IntPolicy>)
        {
            // Scalar fallback for unsupported operations
            for (size_t i = 0; i < size; ++i)
            {
                result[i] = a[i] * b[i];
            }
        }
        else
        {
            for (size_t i = 0; i < simdSize; i += WIDTH)
            {
                auto va = Policy::load(&a[i]);
                auto vb = Policy::load(&b[i]);
                auto vr = Policy::multiply(va, vb);
                Policy::store(&result[i], vr);
            }

            for (size_t i = simdSize; i < size; ++i)
            {
                result[i] = a[i] * b[i];
            }
        }
    }

    /**
     * @brief Multiply array by scalar: result[i] = a[i] * scalar
     */
    template<typename Policy, typename T>
    void multiplyScalarImpl(const T* a, T scalar, T* result, size_t size)
    {
        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        // Check if multiply is supported (SSE2 doesn't support 32-bit int multiply)
        if constexpr (WIDTH == 1 || std::is_same_v<Policy, SSE2IntPolicy>)
        {
            // Scalar fallback for unsupported operations
            for (size_t i = 0; i < size; ++i)
            {
                result[i] = a[i] * scalar;
            }
        }
        else
        {
            auto vScalar = Policy::set1(scalar);

            for (size_t i = 0; i < simdSize; i += WIDTH)
            {
                auto va = Policy::load(&a[i]);
                auto vr = Policy::multiply(va, vScalar);
                Policy::store(&result[i], vr);
            }

            for (size_t i = simdSize; i < size; ++i)
            {
                result[i] = a[i] * scalar;
            }
        }
    }

    /**
     * @brief Sum all elements in array
     */
    template<typename Policy, typename T>
    T sumImpl(const T* data, size_t size)
    {
        if (size == 0) return T(0);

        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        auto vSum = Policy::zero();

        for (size_t i = 0; i < simdSize; i += WIDTH)
        {
            auto v = Policy::load(&data[i]);
            vSum = Policy::add(vSum, v);
        }

        T total = Policy::horizontal_sum(vSum);

        for (size_t i = simdSize; i < size; ++i)
        {
            total += data[i];
        }

        return total;
    }

    /**
     * @brief Find minimum element in array
     */
    template<typename Policy, typename T>
    T minImpl(const T* data, size_t size)
    {
        if (size == 0) return std::numeric_limits<T>::max();

        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        T minVal = data[0];

        // Check if min is supported (SSE2 doesn't support 32-bit signed int min/max)
        if constexpr (WIDTH == 1 || std::is_same_v<Policy, SSE2IntPolicy>)
        {
            // Scalar fallback
            for (size_t i = 1; i < size; ++i)
            {
                minVal = std::min(minVal, data[i]);
            }
        }
        else
        {
            auto vMin = Policy::set1(minVal);

            for (size_t i = 0; i < simdSize; i += WIDTH)
            {
                auto v = Policy::load(&data[i]);
                vMin = Policy::min(vMin, v);
            }

            minVal = Policy::horizontal_min(vMin);

            for (size_t i = simdSize; i < size; ++i)
            {
                minVal = std::min(minVal, data[i]);
            }
        }

        return minVal;
    }

    /**
     * @brief Find maximum element in array
     */
    template<typename Policy, typename T>
    T maxImpl(const T* data, size_t size)
    {
        if (size == 0) return std::numeric_limits<T>::min();

        constexpr size_t WIDTH = Policy::WIDTH;
        const size_t simdSize = (size / WIDTH) * WIDTH;

        T maxVal = data[0];

        // Check if max is supported (SSE2 doesn't support 32-bit signed int min/max)
        if constexpr (WIDTH == 1 || std::is_same_v<Policy, SSE2IntPolicy>)
        {
            // Scalar fallback
            for (size_t i = 1; i < size; ++i)
            {
                maxVal = std::max(maxVal, data[i]);
            }
        }
        else
        {
            auto vMax = Policy::set1(maxVal);

            for (size_t i = 0; i < simdSize; i += WIDTH)
            {
                auto v = Policy::load(&data[i]);
                vMax = Policy::max(vMax, v);
            }

            maxVal = Policy::horizontal_max(vMax);

            for (size_t i = simdSize; i < size; ++i)
            {
                maxVal = std::max(maxVal, data[i]);
            }
        }

        return maxVal;
    }

} // namespace mType::value::simd
