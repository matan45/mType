#pragma once

#include "TypeException.hpp"
#include <string>

namespace errors
{
    /**
     * @brief Thrown when type resolution fails
     *
     * This exception is thrown when the interpreter cannot resolve a type name
     * to an actual type definition, or when type information is missing or invalid.
     */
    class TypeResolutionException : public TypeException
    {
    public:
        explicit TypeResolutionException(const std::string& message,
                                        const SourceLocation& loc = SourceLocation())
            : TypeException(message, loc)
        {
        }

        explicit TypeResolutionException(const std::string& message,
                                        const std::string& typeName,
                                        const SourceLocation& loc = SourceLocation())
            : TypeException("Type resolution error for '" + typeName + "': " + message, loc)
        {
        }
    };
}
