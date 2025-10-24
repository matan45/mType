#pragma once
#include "ScriptException.hpp"
#include <vector>

namespace errors
{
    /**
     * @brief Exception thrown when abstract class rules are violated
     *
     * This exception is thrown for abstract class-related errors such as:
     * - Attempting to instantiate an abstract class
     * - Concrete class not implementing all abstract methods from parent
     * - Abstract method incorrectly implemented
     */
    class AbstractClassException : public ScriptException
    {
    private:
        std::string className;
        std::string methodName;
        std::vector<std::string> unimplementedMethods;

    public:
        explicit AbstractClassException(const std::string& msg,
                                       const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc)
        {
        }

        // Constructor with class context (for instantiation errors)
        AbstractClassException(const std::string& msg,
                             const std::string& abstractClass,
                             const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc),
              className(abstractClass)
        {
        }

        // Constructor with unimplemented methods context
        AbstractClassException(const std::string& msg,
                             const std::string& concreteClass,
                             const std::vector<std::string>& unimplemented,
                             const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc),
              className(concreteClass),
              unimplementedMethods(unimplemented)
        {
        }

        // Constructor with method context
        AbstractClassException(const std::string& msg,
                             const std::string& className,
                             const std::string& method,
                             const SourceLocation& loc = SourceLocation())
            : ScriptException(msg, loc),
              className(className),
              methodName(method)
        {
        }

        const std::string& getClassName() const { return className; }
        const std::string& getMethodName() const { return methodName; }
        const std::vector<std::string>& getUnimplementedMethods() const { return unimplementedMethods; }
    };
}
