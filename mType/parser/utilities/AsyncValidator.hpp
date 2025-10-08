#pragma once

#include "../../ast/GenericType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>
#include <string>

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

        /**
         * @brief Extract the wrapped type from a Promise<T>
         *
         * For example, if type is Promise<Int>, returns Int.
         * If type is not a Promise, returns nullptr.
         *
         * @param promiseType The Promise<T> type to unwrap
         * @return The wrapped type T, or nullptr if not a Promise
         */
        static std::shared_ptr<GenericType> unwrapPromiseType(
            const std::shared_ptr<GenericType>& promiseType);

        /**
         * @brief Wrap a type in Promise<T>
         *
         * Creates a Promise<T> type from a given type T.
         * For example, Int -> Promise<Int>
         *
         * @param innerType The type to wrap
         * @return A new GenericType representing Promise<innerType>
         */
        static std::shared_ptr<GenericType> wrapInPromise(
            const std::shared_ptr<GenericType>& innerType);

        /**
         * @brief Validate that await is used in an async context
         *
         * This validation is performed during parsing via ParseContext.
         * This method provides additional semantic validation if needed.
         *
         * @param isAsyncContext true if currently in an async function
         * @param location Source location for error reporting
         * @throws ParseException if await used outside async context
         */
        static void validateAwaitContext(
            bool isAsyncContext,
            const SourceLocation& location);
    };
}
