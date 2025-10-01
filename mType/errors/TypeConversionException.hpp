#pragma once

#include "TypeException.hpp"
#include <string>

namespace errors
{
    /**
     * @brief Thrown when type conversion fails
     *
     * This exception is thrown when attempting to convert a value from one type
     * to another incompatible type.
     */
    class TypeConversionException : public TypeException
    {
    private:
        std::string sourceType_;
        std::string targetType_;

    public:
        explicit TypeConversionException(const std::string& message,
                               const std::string& sourceType,
                               const std::string& targetType,
                               const SourceLocation& loc = SourceLocation())
            : TypeException("Type conversion error: " + message +
                          " (from '" + sourceType + "' to '" + targetType + "')", loc)
            , sourceType_(sourceType)
            , targetType_(targetType)
        {
        }

        const std::string& getSourceType() const { return sourceType_; }
        const std::string& getTargetType() const { return targetType_; }
    };
}
