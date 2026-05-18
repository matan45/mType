#pragma once
#include "RuntimeException.hpp"
#include <cstddef>
#include <string>

namespace errors
{
    /**
     * Thrown when a class has no constructor matching the given argument
     * list. Mirrors MethodNotFoundException but specialized for <init>.
     * MYT-46.
     */
    class ConstructorNotFoundException : public RuntimeException
    {
    private:
        std::string className_;
        size_t paramCount_;

    public:
        ConstructorNotFoundException(const std::string& className,
                                     size_t paramCount,
                                     const SourceLocation& loc = SourceLocation())
            : RuntimeException(
                "no matching overload for call to constructor of class '" + className
                + "' with " + std::to_string(paramCount) + " argument(s)", loc)
            , className_(className)
            , paramCount_(paramCount)
        {
        }

        const std::string& getClassName() const { return className_; }
        size_t getParamCount() const { return paramCount_; }
    };
}
