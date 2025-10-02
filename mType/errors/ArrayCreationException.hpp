#pragma once

#include "RuntimeException.hpp"
#include <string>
#include <vector>

namespace errors
{
    /**
     * @brief Thrown when array creation fails
     *
     * This exception is thrown when an error occurs during array creation,
     * such as invalid dimensions, negative sizes, or memory allocation failures.
     */
    class ArrayCreationException : public RuntimeException
    {
    private:
        std::string arrayType_;
        std::vector<int> dimensions_;

    public:
        explicit ArrayCreationException(const std::string& message,
                                       const SourceLocation& loc = SourceLocation())
            : RuntimeException("Array creation error: " + message, loc)
        {
        }

        explicit ArrayCreationException(const std::string& message,
                             const std::string& arrayType,
                             const std::vector<int>& dimensions = {},
                             const SourceLocation& loc = SourceLocation())
            : RuntimeException("Array creation error: " + message +
                             " (type: " + arrayType + ")", loc)
            , arrayType_(arrayType)
            , dimensions_(dimensions)
        {
        }

        const std::string& getArrayType() const { return arrayType_; }
        const std::vector<int>& getDimensions() const { return dimensions_; }
    };
}
