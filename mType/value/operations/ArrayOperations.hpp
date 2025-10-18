#pragma once

#include "../NativeArray.hpp"
#include "../ValueType.hpp"
#include <memory>

namespace value::operations
{
    /**
     * @brief High-performance array operations with SIMD support
     *
     * Provides vectorized operations on NativeArray:
     * - Arithmetic operations (add, subtract, multiply, divide, etc.)
     * - Comparison operations (equal, less than, greater than, etc.)
     * - Reduction operations (sum, product, min, max, etc.)
     * - Transformation operations (map, filter, etc.)
     *
     * Automatically uses SIMD when:
     * - Array is SIMD-optimized (int[], float[], bool[])
     * - CPU supports SIMD instructions
     * - Operation is vectorizable
     *
     * Falls back to scalar operations for:
     * - Heterogeneous arrays
     * - Non-SIMD arrays
     * - Non-vectorizable operations
     */
    class ArrayOperations
    {
    public:
        // ========== Arithmetic Operations ==========

        /**
         * Element-wise addition: result[i] = array1[i] + array2[i]
         * SIMD-accelerated for int[] and float[]
         */
        static std::shared_ptr<NativeArray> add(
            const std::shared_ptr<NativeArray>& array1,
            const std::shared_ptr<NativeArray>& array2);

        /**
         * Scalar addition: result[i] = array[i] + scalar
         * SIMD-accelerated for int[] and float[]
         */
        static std::shared_ptr<NativeArray> addScalar(
            const std::shared_ptr<NativeArray>& array,
            const Value& scalar);

        /**
         * Element-wise subtraction: result[i] = array1[i] - array2[i]
         * SIMD-accelerated for int[] and float[]
         */
        static std::shared_ptr<NativeArray> subtract(
            const std::shared_ptr<NativeArray>& array1,
            const std::shared_ptr<NativeArray>& array2);

        /**
         * Element-wise multiplication: result[i] = array1[i] * array2[i]
         * SIMD-accelerated for int[] and float[]
         */
        static std::shared_ptr<NativeArray> multiply(
            const std::shared_ptr<NativeArray>& array1,
            const std::shared_ptr<NativeArray>& array2);

        /**
         * Scalar multiplication: result[i] = array[i] * scalar
         * SIMD-accelerated for int[] and float[]
         */
        static std::shared_ptr<NativeArray> multiplyScalar(
            const std::shared_ptr<NativeArray>& array,
            const Value& scalar);

        // ========== Reduction Operations ==========

        /**
         * Sum all elements in array
         * SIMD-accelerated for int[] and float[]
         * Returns: int for int[], float for float[]
         */
        static Value sum(const std::shared_ptr<NativeArray>& array);

        /**
         * Find minimum value in array
         * SIMD-accelerated for int[] and float[]
         */
        static Value min(const std::shared_ptr<NativeArray>& array);

        /**
         * Find maximum value in array
         * SIMD-accelerated for int[] and float[]
         */
        static Value max(const std::shared_ptr<NativeArray>& array);

        /**
         * Calculate average of all elements
         * SIMD-accelerated for int[] and float[]
         */
        static Value average(const std::shared_ptr<NativeArray>& array);
        
        
        // ========== Utility Operations ==========

        /**
         * Fill array with a constant value
         * SIMD-accelerated for all types
         */
        static void fill(
            const std::shared_ptr<NativeArray>& array,
            const Value& value);

        /**
         * Copy array elements (deep copy)
         * SIMD-accelerated for all types
         */
        static std::shared_ptr<NativeArray> copy(
            const std::shared_ptr<NativeArray>& source);

        /**
         * Reverse array elements in-place
         * SIMD-accelerated for all types
         */
        static void reverse(const std::shared_ptr<NativeArray>& array);
    
    private:
        // Helper to check if arrays are compatible for binary operations
        static bool areCompatible(
            const std::shared_ptr<NativeArray>& array1,
            const std::shared_ptr<NativeArray>& array2);

        // Helper to extract scalar value
        static bool extractInt(const Value& val, int& out);
        static bool extractFloat(const Value& val, float& out);
        static bool extractBool(const Value& val, bool& out);

        // Template helper for binary operations (add, subtract, multiply)
        template<typename IntSimdOp, typename FloatSimdOp, typename ScalarOp>
        static std::shared_ptr<NativeArray> performBinaryOperation(
            const std::shared_ptr<NativeArray>& array1,
            const std::shared_ptr<NativeArray>& array2,
            IntSimdOp intSimdOp,
            FloatSimdOp floatSimdOp,
            ScalarOp scalarOp,
            const char* operationName);

        // Template helper for scalar operations (addScalar, multiplyScalar)
        template<typename IntSimdOp, typename FloatSimdOp, typename ScalarOp>
        static std::shared_ptr<NativeArray> performScalarOperation(
            const std::shared_ptr<NativeArray>& array,
            const Value& scalar,
            IntSimdOp intSimdOp,
            FloatSimdOp floatSimdOp,
            ScalarOp scalarOp,
            const char* operationName);

        // Template helper for reduction operations (sum, min, max)
        template<typename IntSimdOp, typename FloatSimdOp, typename ScalarOp>
        static Value performReduction(
            const std::shared_ptr<NativeArray>& array,
            IntSimdOp intSimdOp,
            FloatSimdOp floatSimdOp,
            ScalarOp scalarOp,
            const char* operationName,
            bool allowEmpty = false);
    };

} // namespace value::operations
