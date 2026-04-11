#pragma once
#include "RuntimeException.hpp"
#include <string>

namespace errors
{
    /**
     * Thrown when a free function lookup fails at runtime. Mirrors
     * MethodNotFoundException but specialized for top-level functions
     * (no enclosing class). MYT-46.
     */
    class FunctionNotFoundException : public RuntimeException
    {
    private:
        std::string functionName_;

    public:
        explicit FunctionNotFoundException(const std::string& functionName,
                                           const SourceLocation& loc = SourceLocation())
            : RuntimeException("Function not found: " + functionName, loc)
            , functionName_(functionName)
        {
        }

        const std::string& getFunctionName() const { return functionName_; }
    };
}
