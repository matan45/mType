#pragma once

#include "RuntimeException.hpp"
#include <string>

namespace errors
{
    /**
     * @brief Thrown when a field cannot be found on a class or object
     *
     * This exception is thrown when attempting to access a field that does not exist
     * or is not accessible on a class or object instance.
     */
    class FieldNotFoundException : public RuntimeException
    {
    private:
        std::string fieldName_;
        std::string className_;

    public:
        explicit FieldNotFoundException(const std::string& fieldName,
                                       const SourceLocation& loc = SourceLocation())
            : RuntimeException("Field '" + fieldName + "' not found", loc)
            , fieldName_(fieldName)
        {
        }

        FieldNotFoundException(const std::string& fieldName,
                              const std::string& className,
                              const SourceLocation& loc = SourceLocation())
            : RuntimeException("Field '" + fieldName + "' not found" +
                             (className.empty() ? "" : " on class '" + className + "'"), loc)
            , fieldName_(fieldName)
            , className_(className)
        {
        }

        const std::string& getFieldName() const { return fieldName_; }
        const std::string& getClassName() const { return className_; }
    };
}
