#pragma once

#include "RuntimeException.hpp"
#include <string>

namespace errors
{
    /**
     * @brief Thrown when a class cannot be found in the registry
     *
     * This exception is thrown when attempting to reference, instantiate, or
     * access a class that has not been defined or is not in scope.
     */
    class ClassNotFoundException : public RuntimeException
    {
    private:
        std::string className_;

    public:
        explicit ClassNotFoundException(const std::string& className,
                                       const SourceLocation& loc = SourceLocation())
            : RuntimeException("Class '" + className + "' not found", loc)
            , className_(className)
        {
        }

        const std::string& getClassName() const { return className_; }
    };
}
