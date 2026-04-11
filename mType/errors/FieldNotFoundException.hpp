#pragma once

#include "RuntimeException.hpp"
#include <string>
#include <vector>

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
        // MYT-35 Phase 4 — names of fields that DO exist on the target
        // class. The converter runs Levenshtein over this for "did you
        // mean 'count'?" suggestions.
        std::vector<std::string> identifierPool_;

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

        FieldNotFoundException(const std::string& fieldName,
                                const std::string& className,
                                const SourceLocation& loc,
                                std::vector<std::string> pool)
            : RuntimeException("Field '" + fieldName + "' not found" +
                             (className.empty() ? "" : " on class '" + className + "'"), loc)
            , fieldName_(fieldName)
            , className_(className)
            , identifierPool_(std::move(pool))
        {
        }

        const std::string& getFieldName() const { return fieldName_; }
        const std::string& getClassName() const { return className_; }
        const std::vector<std::string>& getIdentifierPool() const { return identifierPool_; }
    };
}
