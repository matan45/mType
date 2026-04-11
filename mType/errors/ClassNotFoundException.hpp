#pragma once

#include "RuntimeException.hpp"
#include <string>
#include <vector>

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
        // MYT-35 Phase 4 — see UndefinedException for the rationale.
        // Throwers that know the candidate class names visible at the
        // failure site can attach them so the diagnostic converter can
        // surface a "did you mean 'Foo'?" suggestion.
        std::vector<std::string> identifierPool_;

    public:
        explicit ClassNotFoundException(const std::string& className,
                                       const SourceLocation& loc = SourceLocation())
            : RuntimeException("Class '" + className + "' not found", loc)
            , className_(className)
        {
        }

        ClassNotFoundException(const std::string& className,
                                const SourceLocation& loc,
                                std::vector<std::string> pool)
            : RuntimeException("Class '" + className + "' not found", loc)
            , className_(className)
            , identifierPool_(std::move(pool))
        {
        }

        const std::string& getClassName() const { return className_; }
        const std::vector<std::string>& getIdentifierPool() const { return identifierPool_; }
    };
}
