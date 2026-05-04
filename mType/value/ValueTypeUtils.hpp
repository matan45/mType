#pragma once
#include "ValueType.hpp"
#include <cstddef>

namespace value {
    /**
     * @brief Utility class for ValueType operations and conversions
     *
     * Separates value type utility logic from the core ValueType definitions
     * following Single Responsibility Principle. This allows ValueType.hpp to
     * focus on defining types while this utility handles type operations.
     *
     * Design Principles:
     * - Single Responsibility: Only handles value type utilities
     * - Stateless: All methods are static (no instance state)
     * - Type Safety: Provides type-safe conversions from Value to ValueType
     */
    class ValueTypeUtils {
    public:
        /**
         * @brief Get the ValueType enum from a Value variant
         * @param value The Value variant to inspect
         * @return The corresponding ValueType enum
         *
         * This function inspects the runtime type stored in a Value variant
         * and returns the corresponding ValueType enum. It handles all
         * standard types including primitives, objects, arrays, lambdas,
         * and multi-dimensional arrays.
         *
         * Special handling for:
         * - Multi-dimensional arrays (FlatMultiArray, SparseMultiArray, FlatMultiObjectArray)
         * - Lambda values (both LambdaValue and BytecodeLambda)
         * - Promise values (treated as OBJECT type)
         * - Null values (nullptr_t mapped to NULL_TYPE)
         * - String types (both std::string and InternedString)
         */
        static ValueType getValueType(const Value& value);
    };
}
