#include "ParserValidator.hpp"
#include <cctype>

namespace parser
{
    bool ParserValidator::isValidClassName(const std::string& name)
    {
        return !name.empty() && std::isupper(name[0]);
    }

    bool ParserValidator::isValidNamespaceName(const std::string& name)
    {
        return !name.empty() && std::islower(name[0]);
    }
}
