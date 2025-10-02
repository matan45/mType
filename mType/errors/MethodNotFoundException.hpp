#pragma once

#include "RuntimeException.hpp"
#include <string>

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

        const std::string& getMethodName() const { return methodName_; }
        const std::string& getClassName() const { return className_; }
    };
}
