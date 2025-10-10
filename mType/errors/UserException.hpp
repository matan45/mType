#pragma once
#include "ScriptException.hpp"
#include "../value/ValueType.hpp"
#include <memory>
#include <vector>
#include <string>
#include <variant>

// Forward declaration to avoid circular dependency
namespace runtimeTypes::klass {
    class ObjectInstance;
}

namespace errors
{
    /**
     * @brief Exception thrown by user code via the 'throw' statement
     *
     * Wraps an mType exception object (e.g., instance of Exception class)
     * Provides stack trace and source location information for debugging
     */
    class UserException : public ScriptException
    {
    private:
        value::Value exceptionValue;  // The actual exception object/value thrown
        std::string exceptionTypeName; // Type name of the exception
        std::vector<std::string> stackTrace; // Call stack at throw point

    public:
        explicit UserException(const value::Value& excValue,
                              const std::string& typeName,
                              const SourceLocation& loc = SourceLocation())
            : ScriptException("User exception thrown: " + typeName, loc),
              exceptionValue(excValue),
              exceptionTypeName(typeName)
        {
        }

        const value::Value& getExceptionValue() const
        {
            return exceptionValue;
        }

        const std::string& getExceptionTypeName() const
        {
            return exceptionTypeName;
        }

        void setStackTrace(const std::vector<std::string>& trace)
        {
            stackTrace = trace;
        }

        const std::vector<std::string>& getStackTrace() const
        {
            return stackTrace;
        }

        void addStackFrame(const std::string& frame)
        {
            stackTrace.push_back(frame);
        }

        /**
         * @brief Check if this exception can be caught by a catch block of the given type
         *
         * @param catchType The exception type specified in the catch clause
         * @return true if this exception matches the catch type (including inheritance)
         */
        bool matchesCatchType(const std::string& catchType) const;
    };
}
