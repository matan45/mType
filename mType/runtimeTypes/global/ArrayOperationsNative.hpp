#pragma once

#include "../../value/ValueType.hpp"
#include "../../environment/Environment.hpp"
#include <memory>

namespace runtimeTypes::global
{
    /**
     * @brief Native functions for high-performance array operations
     *
     * Registers SIMD-accelerated array operations as native functions
     * in the mType environment, making them available to user code.
     *
     * Usage in mType:
     *   int[] a = new int[100];
     *   int[] b = new int[100];
     *   int[] result = arrayAdd(a, b);  // SIMD-accelerated
     *   int sum = arraySum(a);          // SIMD-accelerated
     */
    class ArrayOperationsNative
    {
    public:
        /**
         * Register all array operation functions in the environment
         */
        static void registerAll(std::shared_ptr<environment::Environment> env);

    private:
        // Arithmetic operations
        static value::Value arrayAdd(const std::vector<value::Value>& args);
        static value::Value arrayAddScalar(const std::vector<value::Value>& args);
        static value::Value arraySubtract(const std::vector<value::Value>& args);
        static value::Value arrayMultiply(const std::vector<value::Value>& args);
        static value::Value arrayMultiplyScalar(const std::vector<value::Value>& args);

        // Reduction operations
        static value::Value arraySum(const std::vector<value::Value>& args);
        static value::Value arrayMin(const std::vector<value::Value>& args);
        static value::Value arrayMax(const std::vector<value::Value>& args);
        static value::Value arrayAverage(const std::vector<value::Value>& args);

        // Utility operations
        static value::Value arrayFill(const std::vector<value::Value>& args);
        static value::Value arrayCopy(const std::vector<value::Value>& args);
        static value::Value arrayReverse(const std::vector<value::Value>& args);
    };

} // namespace runtimeTypes::global
