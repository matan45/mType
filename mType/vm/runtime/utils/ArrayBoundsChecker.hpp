#pragma once
#include <string>
#include <cstddef>
#include "../context/ExecutionContext.hpp"
#include "../../../environment/Environment.hpp"
#include "ErrorLocationHelper.hpp"

namespace vm::runtime::utils
{
    /**
     * Utility class for array bounds checking
     * Eliminates duplication of bounds validation logic across array operations
     */
    class ArrayBoundsChecker
    {
    public:
        /**
         * Checks if an index is within array bounds and throws an error if not
         * @param context Execution context for error location
         * @param index The array index to check
         * @param arraySize The size of the array
         * @param arrayType Type of array for error message (e.g., "Array", "FlatMultiArray")
         * @throws errors::RuntimeException if index is out of bounds
         */
        static void checkBounds(const ExecutionContext& context,
                              const std::shared_ptr<environment::Environment>& env,
                              int index, size_t arraySize,
                              const std::string& arrayType = "Array")
        {
            if (index < 0 || static_cast<size_t>(index) >= arraySize) {
                ErrorLocationHelper::throwUserException(context, env,
                    "IndexOutOfBoundsException",
                    arrayType + " index out of bounds: " + std::to_string(index) +
                    " for array of size " + std::to_string(arraySize));
            }
        }

        /**
         * Checks if an index is within array bounds (template version for type safety)
         * @tparam ArrayType Type of array (must have size() method)
         * @param context Execution context for error location
         * @param env Environment for resolving exception type
         * @param index The array index to check
         * @param array The array to check bounds against
         * @param arrayTypeName Type of array for error message
         * @throws errors::RuntimeException if index is out of bounds
         */
        template<typename ArrayType>
        static void checkBounds(const ExecutionContext& context,
                              const std::shared_ptr<environment::Environment>& env,
                              int index, const ArrayType& array,
                              const std::string& arrayTypeName = "Array")
        {
            checkBounds(context, env, index, array->size(), arrayTypeName);
        }
    };
}
