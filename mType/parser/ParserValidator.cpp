#include "ParserValidator.hpp"
#include <cctype>

namespace parser
{
    bool ParserValidator::isValidClassName(std::string_view name)
    {
        return !name.empty() && std::isupper(name[0]);
    }

    bool ParserValidator::isValidNamespaceName(std::string_view name)
    {
        return !name.empty() && std::islower(name[0]);
    }

    bool ParserValidator::isValidParentClassName(std::string_view name)
    {
        // Parent class names follow same convention as regular class names
        // Must start with uppercase letter
        if (name.empty() || !std::isupper(name[0])) {
            return false;
        }

        // Extract base name (before any generic parameters)
        size_t genericStart = name.find('<');
        std::string_view baseName = (genericStart != std::string_view::npos)
            ? name.substr(0, genericStart)
            : name;

        // Validate base name - must be all alphanumeric after first uppercase
        for (char c : baseName) {
            if (!std::isalnum(c)) {
                return false;
            }
        }

        return true;
    }
}
