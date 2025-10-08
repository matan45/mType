#pragma once
#include "ValueType.hpp"
#include <memory>
#include <functional>
#include <vector>

namespace value
{
    /**
     * @brief State of a Promise
     */
    enum class PromiseState
    {
        PENDING,   // Promise is not yet resolved
        FULFILLED, // Promise resolved successfully with a value
        REJECTED   // Promise rejected with an error (future use)
    };

    /**
     * @brief Runtime representation of a Promise<T>
     *
     * For Phase 2 (Evaluator), we use a simple synchronous model:
     * - await immediately resolves the promise and returns its value
     * - Promises are always in FULFILLED state after async function execution
     *
     * This is a simplified model suitable for single-threaded cooperative execution.
     * True asynchronous behavior would require event loops and scheduling.
     */
    class PromiseValue
    {
    private:
        PromiseState state;
        Value value;
        std::string errorMessage; // For future error handling

    public:
        /**
         * @brief Create a pending promise
         */
        PromiseValue()
            : state(PromiseState::PENDING)
            , value(std::monostate{})
        {
        }

        /**
         * @brief Create a fulfilled promise with a value
         */
        explicit PromiseValue(const Value& val)
            : state(PromiseState::FULFILLED)
            , value(val)
        {
        }

        /**
         * @brief Get the current state of the promise
         */
        PromiseState getState() const { return state; }

        /**
         * @brief Check if promise is fulfilled
         */
        bool isFulfilled() const { return state == PromiseState::FULFILLED; }

        /**
         * @brief Check if promise is pending
         */
        bool isPending() const { return state == PromiseState::PENDING; }

        /**
         * @brief Check if promise is rejected
         */
        bool isRejected() const { return state == PromiseState::REJECTED; }

        /**
         * @brief Resolve the promise with a value
         */
        void resolve(const Value& val)
        {
            if (state == PromiseState::PENDING)
            {
                state = PromiseState::FULFILLED;
                value = val;
            }
        }

        /**
         * @brief Reject the promise with an error (future use)
         */
        void reject(const std::string& error)
        {
            if (state == PromiseState::PENDING)
            {
                state = PromiseState::REJECTED;
                errorMessage = error;
            }
        }

        /**
         * @brief Get the resolved value
         * @throws std::runtime_error if promise is not fulfilled
         */
        Value getValue() const
        {
            if (state != PromiseState::FULFILLED)
            {
                throw std::runtime_error("Cannot get value from non-fulfilled promise");
            }
            return value;
        }

        /**
         * @brief Get error message if rejected
         */
        std::string getError() const
        {
            return errorMessage;
        }
    };
}
