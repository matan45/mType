#pragma once
#include "ScriptException.hpp"

namespace errors
{
    /**
     * @brief Exception thrown when inheritance rules are violated
     *
     * This exception is thrown for inheritance-related errors such as:
     * - Circular inheritance detected
     * - Parent class not found
     * - Invalid method override (signature mismatch)
     * - Invalid super() call (wrong position, wrong arguments)
     * - Multiple inheritance attempted (not supported)
     */
    class InheritanceException : public ScriptException
    {
    private:
        std::string childClassName;
        std::string parentClassName;
        std::string methodName;

    public:
        explicit InheritanceException(const std::string& msg,
                                     const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }

        // Constructor with child and parent class context
        InheritanceException(const std::string& msg,
                           const std::string& childClass,
                           const std::string& parentClass,
                           const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc),
              childClassName(childClass),
              parentClassName(parentClass)
        {
        }

        // Constructor with method override context
        InheritanceException(const std::string& msg,
                           const std::string& childClass,
                           const std::string& parentClass,
                           const std::string& method,
                           const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc),
              childClassName(childClass),
              parentClassName(parentClass),
              methodName(method)
        {
        }

        const std::string& getChildClassName() const { return childClassName; }
        const std::string& getParentClassName() const { return parentClassName; }
        const std::string& getMethodName() const { return methodName; }
    };
}
