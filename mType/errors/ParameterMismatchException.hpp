#pragma once

#include "RuntimeException.hpp"
#include <string>

namespace errors
{
    /**
     * @brief Thrown when function/method parameters don't match the signature
     *
     * This exception is thrown when the number or types of arguments provided
     * to a function or method call don't match the expected parameters.
     */
    class ParameterMismatchException : public RuntimeException
    {
    private:
        std::string functionName_;
        int expectedCount_;
        int actualCount_;

    public:
        ParameterMismatchException(const std::string& functionName,
                                  int expectedCount,
                                  int actualCount,
                                  const SourceLocation& loc = SourceLocation())
            : RuntimeException("Parameter mismatch for '" + functionName + "': expected " +
                             std::to_string(expectedCount) + " parameters, got " +
                             std::to_string(actualCount), loc)
            , functionName_(functionName)
            , expectedCount_(expectedCount)
            , actualCount_(actualCount)
        {
        }

        const std::string& getFunctionName() const { return functionName_; }
        int getExpectedCount() const { return expectedCount_; }
        int getActualCount() const { return actualCount_; }
    };
}
