#pragma once
#include <string_view>
#include "../../errors/SourceLocation.hpp"

namespace parser
{
    /// @brief Specialized utility for validating identifier and type names
    /// Handles naming convention validation for variables, functions, classes, interfaces, etc.
    /// Follows Single Responsibility Principle - only handles name validation
    class NameValidator
    {
    public:
        /// @brief Check if a string represents a valid identifier
        /// @param name String to validate
        /// @return true if valid identifier, false otherwise
        [[nodiscard]] static bool isValidIdentifier(std::string_view name) noexcept;

        /// @brief Validate function/method naming convention (must start with lowercase)
        /// @param name Function or method name to validate
        /// @param isStatic Whether this is a static method
        /// @param context Description for error message (e.g., "method", "function", "static method")
        /// @throws ParseException if name doesn't follow lowercase convention
        static void validateFunctionNamingConvention(std::string_view name, bool isStatic,
                                                     std::string_view context, const errors::SourceLocation& location);

        /// @brief Validate that a name starts with capital letter (for classes/interfaces)
        /// @param name Name to validate
        /// @param context Description for error message (e.g., "Class", "Interface")
        /// @param location Source location for error reporting
        /// @throws ParseException if name doesn't start with capital letter
        static void validateCapitalizedName(std::string_view name,
                                            std::string_view context,
                                            const errors::SourceLocation& location);

        /// @brief Validate that a name is a valid identifier (for variables/fields/parameters)
        /// @param name Name to validate
        /// @param context Description for error message (e.g., "Variable", "Field", "Parameter")
        /// @param location Source location for error reporting
        /// @throws ParseException if name is not a valid identifier
        static void validateIdentifierName(std::string_view name,
                                           std::string_view context,
                                           const errors::SourceLocation& location);

    private:
        // Utility class - no instances allowed
        NameValidator() = delete;
        ~NameValidator() = delete;
        NameValidator(const NameValidator&) = delete;
        NameValidator& operator=(const NameValidator&) = delete;
    };
}
