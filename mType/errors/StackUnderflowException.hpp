#pragma once
#include "RuntimeException.hpp"
#include <cstddef>
#include <string>

namespace errors
{
    /**
     * Thrown by the VM when a bytecode instruction or helper tries to pop
     * more values than the stack holds. Usually indicates a codegen bug
     * or malformed bytecode. MYT-46.
     */
    class StackUnderflowException : public RuntimeException
    {
    private:
        size_t required_;
        size_t available_;

    public:
        StackUnderflowException(size_t required, size_t available,
                                const SourceLocation& loc = SourceLocation())
            : RuntimeException(
                "Stack underflow: required " + std::to_string(required)
                + " but only " + std::to_string(available) + " available", loc)
            , required_(required)
            , available_(available)
        {
        }

        size_t getRequired()  const { return required_; }
        size_t getAvailable() const { return available_; }
    };
}
