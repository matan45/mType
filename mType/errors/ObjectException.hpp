#pragma once

#include "RuntimeException.hpp"
#include <string>

namespace errors
{
    /**
     * @brief Thrown when an operation on an object fails
     *
     * This exception is thrown when an operation on an object instance fails,
     * such as invalid method calls, field access, or type mismatches.
     */
    class ObjectException : public RuntimeException
    {
    private:
        std::string objectType_;

    public:
        explicit ObjectException(const std::string& message,
                                const SourceLocation& loc = SourceLocation())
            : RuntimeException(message, loc)
        {
        }

        ObjectException(const std::string& message,
                       const std::string& objectType,
                       const SourceLocation& loc = SourceLocation())
            : RuntimeException(message + " (object type: " + objectType + ")", loc)
            , objectType_(objectType)
        {
        }

        // Overload for compatibility with 3-parameter calls
        ObjectException(const std::string& message,
                       const std::string& objectType,
                       const std::string& /*location*/)
            : RuntimeException(message + " (object type: " + objectType + ")")
            , objectType_(objectType)
        {
        }

        const std::string& getObjectType() const { return objectType_; }
    };
}
