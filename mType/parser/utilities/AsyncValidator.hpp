#pragma once

#include "../../ast/GenericType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>

namespace parser::utilities
{
    using namespace ast;
    using namespace errors;

    /**
     * @brief Utility class for validating async/await usage in mType
     *
     * Provides validation logic to ensure:
     * - Async functions have Promise<T> return types
     * - Await expressions only appear in async contexts
     * - Type consistency for async operations
     */
    class AsyncValidator
    {
    public:
        /**
         * @brief Validate that a return type is valid for an async function
         *
         * Async functions should return Promise<T> where T is the actual return type.
         * This method checks if the return type follows this convention.
         *
         * @param returnType The return type to validate
         * @param location Source location for error reporting
         * @throws ParseException if return type is not Promise<T>
         */
        static void validateAsyncReturnType(
            const std::shared_ptr<GenericType>& returnType,
            const SourceLocation& location);

        /**
         * @brief Check if a type is a Promise<T> type
         *
         * @param type The type to check
         * @return true if type is Promise<T>, false otherwise
         */
        static bool isPromiseType(const std::shared_ptr<GenericType>& type);
    };
}
