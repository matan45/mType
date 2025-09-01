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
}
