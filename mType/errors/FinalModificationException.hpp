#pragma once

#include "RuntimeException.hpp"
#include <string>

namespace errors
{
    /**
     * @brief Thrown when attempting to modify a final field or variable
     *
     * This exception is thrown when code attempts to reassign a value to
     * a field or variable that has been declared as final (immutable).
     */
    class FinalModificationException : public RuntimeException
    {
    private:
        std::string memberName_;
        std::string className_;

    public:
        explicit FinalModificationException(const std::string& memberName,
                                           const SourceLocation& loc = SourceLocation())
            : RuntimeException("Cannot modify final member '" + memberName + "'", loc)
            , memberName_(memberName)
        {
        }

        FinalModificationException(const std::string& memberName,
                                  const std::string& className,
                                  const SourceLocation& loc = SourceLocation())
            : RuntimeException("Cannot modify final member '" + memberName + "'" +
                             (className.empty() ? "" : " on class '" + className + "'"), loc)
            , memberName_(memberName)
            , className_(className)
        {
        }

        const std::string& getMemberName() const { return memberName_; }
        const std::string& getClassName() const { return className_; }
    };
}
