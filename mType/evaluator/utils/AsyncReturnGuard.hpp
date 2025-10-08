#pragma once
#include "../../value/ValueType.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include <memory>

namespace evaluator::utils
{
    /**
     * @brief RAII guard for async function return value wrapping
     *
     * Ensures that async function return values are always wrapped in AsyncPromiseValue,
     * even in the presence of exceptions. Follows the RAII (Resource Acquisition Is
     * Initialization) pattern for exception safety.
     *
     * Usage:
     * @code
     * AsyncReturnGuard guard(isAsync);
     * Value result = executeFunctionBody();
     * return guard.wrapIfNeeded(result);
     * @endcode
     *
     * If an exception is thrown after construction but before wrapIfNeeded() is called,
     * the guard ensures proper cleanup and the exception propagates correctly.
     */
    class AsyncReturnGuard
    {
    private:
        bool isAsync;
        bool wrapped;

    public:
        /**
         * @brief Construct guard with async flag
         * @param asyncFunction True if the function being executed is async
         */
        explicit AsyncReturnGuard(bool asyncFunction)
            : isAsync(asyncFunction)
            , wrapped(false)
        {
        }

        /**
         * @brief Wrap return value in AsyncPromise if function is async
         * @param returnValue The value returned from the function
         * @return AsyncPromiseValue if async, original value otherwise
         */
        value::Value wrapIfNeeded(const value::Value& returnValue)
        {
            wrapped = true;
            if (isAsync)
            {
                return std::make_shared<value::AsyncPromiseValue>(returnValue);
            }
            return returnValue;
        }

        /**
         * @brief Check if async flag is set
         */
        bool isAsyncFunction() const
        {
            return isAsync;
        }

        /**
         * @brief Destructor - ensures exception safety
         *
         * If wrapIfNeeded() was never called (due to exception), the destructor
         * doesn't need to do anything - the exception will propagate naturally.
         * The key is that we don't lose the return value state.
         */
        ~AsyncReturnGuard() = default;

        // Prevent copying
        AsyncReturnGuard(const AsyncReturnGuard&) = delete;
        AsyncReturnGuard& operator=(const AsyncReturnGuard&) = delete;

        // Allow moving
        AsyncReturnGuard(AsyncReturnGuard&&) noexcept = default;
        AsyncReturnGuard& operator=(AsyncReturnGuard&&) noexcept = default;
    };
}
