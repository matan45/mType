#include "StackManager.hpp"
#include <cstddef>
#include <string>
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    // Hot methods live inline in the header (MYT-318). This file keeps only
    // the cold-path operations and the underflow-exception construction so
    // the inline checkStackUnderflow stays small enough for the optimizer
    // to fold into surrounding hot code.

    void StackManager::resize(size_t newSize) {
        stack.resize(newSize);
    }

    void StackManager::resize(size_t newSize, const value::Value& defaultValue) {
        stack.resize(newSize, defaultValue);
    }

    void StackManager::throwStackUnderflow(size_t required) const {
        throw errors::RuntimeException("Stack underflow: required " +
            std::to_string(required) + " values but stack has " +
            std::to_string(stack.size()));
    }
}
