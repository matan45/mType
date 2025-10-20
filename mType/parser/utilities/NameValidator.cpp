#include "NameValidator.hpp"
#include "../../errors/ParseException.hpp"
#include <cctype>

namespace parser
{
    bool NameValidator::isValidIdentifier(std::string_view name) noexcept
    {
        if (name.empty()) return false;

        // First character must be letter or underscore
        if (!std::isalpha(name[0]) && name[0] != '_') return false;

        // Remaining characters must be alphanumeric or underscore
        for (size_t i = 1; i < name.size(); ++i) {
            if (!std::isalnum(name[i]) && name[i] != '_') return false;
        }

        return true;
    }

    void NameValidator::validateFunctionNamingConvention(std::string_view name, bool isStatic,
                                                         std::string_view context, const errors::SourceLocation& location)
    {
        using namespace errors;

        if (name.empty()) {
            throw ParseException(std::string(context) + " name cannot be empty", location);
        }

        // Check if first character is lowercase letter
        if (!std::islower(name[0]) && name[0] != '_') {
            std::string message = std::string(context) + " '" + std::string(name) +
                                "' must start with a lowercase letter. " +
                                (isStatic ? "Static methods" : "Functions and methods") +
                                " should follow camelCase convention.";
            throw ParseException(message, location);
        }

        // Additional validation: ensure it's a valid identifier
        if (!isValidIdentifier(name)) {
            throw ParseException(std::string(context) + " '" + std::string(name) + "' is not a valid identifier", location);
        }
    }

    void NameValidator::validateCapitalizedName(std::string_view name,
                                               std::string_view context,
                                               const errors::SourceLocation& location)
    {
        using namespace errors;

        if (name.empty())
        {
            std::string message = std::string(context) + " name cannot be empty";
            throw ParseException(message, location);
        }

        if (!std::isupper(name[0]))
        {
            std::string message = std::string(context) + " name '" + std::string(name) +
                                "' must start with a capital letter";
            throw ParseException(message, location);
        }

        // Ensure it's a valid identifier
        if (!isValidIdentifier(name))
        {
            std::string message = std::string(context) + " name '" + std::string(name) +
                                "' is not a valid identifier. Names can only contain letters, digits, and underscores.";
            throw ParseException(message, location);
        }
    }

    void NameValidator::validateIdentifierName(std::string_view name,
                                               std::string_view context,
                                               const errors::SourceLocation& location)
    {
        using namespace errors;

        if (name.empty())
        {
            std::string message = std::string(context) + " name cannot be empty";
            throw ParseException(message, location);
        }

        // Ensure it's a valid identifier (no special characters)
        if (!isValidIdentifier(name))
        {
            std::string message = std::string(context) + " name '" + std::string(name) +
                                "' is not a valid identifier. Names can only contain letters, digits, and underscores, "
                                "and must start with a letter or underscore.";
            throw ParseException(message, location);
        }
    }
}
