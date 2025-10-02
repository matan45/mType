#pragma once

#include "RuntimeException.hpp"
#include <string>

namespace errors
{
    /**
     * @brief Thrown when attempting to access a null pointer
     *
     * This exception is thrown when code attempts to dereference a null pointer
     * or access members/methods on a null object.
     */
    class NullPointerException : public RuntimeException
    {
    private:
        std::string operation_;

    public:
        explicit NullPointerException(const std::string& message,
                                     const SourceLocation& loc = SourceLocation())
            : RuntimeException(message, loc)
        {
        }

        NullPointerException(const std::string& message,
                           const std::string& operation,
                           const SourceLocation& loc = SourceLocation())
            : RuntimeException("Null pointer exception during " + operation + ": " + message, loc)
            , operation_(operation)
        {
        }

        const std::string& getOperation() const { return operation_; }
    };
}
