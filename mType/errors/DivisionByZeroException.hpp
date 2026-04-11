#pragma once
#include "RuntimeException.hpp"
#include <string>

namespace errors
{
    /**
     * Thrown by arithmetic operators when the right-hand operand is zero.
     * Covers integer /, integer %, and float /. MYT-46.
     */
    class DivisionByZeroException : public RuntimeException
    {
    private:
        std::string operation_;  // "division" | "modulo"

    public:
        explicit DivisionByZeroException(const std::string& operation = "division",
                                         const SourceLocation& loc = SourceLocation())
            : RuntimeException(operation == "modulo" ? "Modulo by zero" : "Division by zero", loc)
            , operation_(operation)
        {
        }

        const std::string& getOperation() const { return operation_; }
    };
}
