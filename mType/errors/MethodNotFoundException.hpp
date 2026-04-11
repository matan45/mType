#pragma once

#include "RuntimeException.hpp"
#include <string>
#include <vector>

namespace errors
{
    /**
     * @brief Thrown when a method cannot be found on a class or object
     *
     * This exception is thrown when attempting to call a method that does not exist
     * or is not accessible on a class or object instance.
     */
    class MethodNotFoundException : public RuntimeException
    {
    private:
        std::string methodName_;
        std::string className_;
        // MYT-35 Phase 4 — names of methods that DO exist on the target
        // class. Used by the diagnostic converter to power "did you mean
        // 'doStuff'?" suggestions.
        std::vector<std::string> identifierPool_;

    public:
        MethodNotFoundException(const std::string& methodName,
                               const std::string& className,
                               const SourceLocation& loc = SourceLocation())
            : RuntimeException("Method '" + methodName + "' not found" +
                             (className.empty() ? "" : " on class '" + className + "'"), loc)
            , methodName_(methodName)
            , className_(className)
        {
        }

        // Overload for compatibility with 3-parameter calls
        MethodNotFoundException(const std::string& methodName,
                               const std::string& className,
                               const std::string& /*location*/)
            : RuntimeException("Method '" + methodName + "' not found" +
                             (className.empty() ? "" : " on class '" + className + "'"))
            , methodName_(methodName)
            , className_(className)
        {
        }

        MethodNotFoundException(const std::string& methodName,
                                const std::string& className,
                                const SourceLocation& loc,
                                std::vector<std::string> pool)
            : RuntimeException("Method '" + methodName + "' not found" +
                             (className.empty() ? "" : " on class '" + className + "'"), loc)
            , methodName_(methodName)
            , className_(className)
            , identifierPool_(std::move(pool))
        {
        }

        const std::string& getMethodName() const { return methodName_; }
        const std::string& getClassName() const { return className_; }
        const std::vector<std::string>& getIdentifierPool() const { return identifierPool_; }
    };
}
