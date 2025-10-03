#include "AccessViolationException.hpp"
#include "../ast/AccessModifier.hpp"
#include <sstream>

namespace errors
{
    // Static helper for initial message formatting
    static std::string formatMessage(
        const std::string& member,
        const std::string& type,
        ast::AccessModifier modifier,
        const std::string& targetClass,
        const std::string& callingFrom)
    {
        std::ostringstream oss;

        oss << "Cannot access " << ast::accessModifierToString(modifier)
            << " " << type << " '" << member << "' of class '"
            << targetClass << "'";

        if (!callingFrom.empty() && callingFrom != "global scope")
        {
            oss << " from class '" << callingFrom << "'";
        }
        else if (callingFrom == "global scope")
        {
            oss << " from global scope";
        }

        return oss.str();
    }

    AccessViolationException::AccessViolationException(
        const std::string& member,
        const std::string& type,
        ast::AccessModifier modifier,
        const std::string& targetClass,
        const std::string& callingFrom,
        const SourceLocation& loc)
        : TypeException(formatMessage(member, type, modifier, targetClass, callingFrom), loc),
          memberName(member),
          memberType(type),
          accessLevel(modifier),
          targetClassName(targetClass),
          callingContext(callingFrom)
    {
    }

    std::string AccessViolationException::formatDetailedMessage() const
    {
        std::ostringstream oss;

        oss << "Cannot access " << ast::accessModifierToString(accessLevel)
            << " " << memberType << " '" << memberName << "' of class '"
            << targetClassName << "'";

        if (!callingContext.empty() && callingContext != "global scope")
        {
            oss << " from class '" << callingContext << "'";
        }
        else if (callingContext == "global scope")
        {
            oss << " from global scope";
        }

        oss << ".";

        // Add helpful suggestion
        if (accessLevel == ast::AccessModifier::PRIVATE)
        {
            oss << "\nSuggestion: Make the " << memberType
                << " public or add a public accessor method.";
        }
        else if (accessLevel == ast::AccessModifier::PROTECTED)
        {
            oss << "\nNote: Protected members are only accessible from '"
                << targetClassName << "' or its subclasses.";
        }

        return oss.str();
    }
}
