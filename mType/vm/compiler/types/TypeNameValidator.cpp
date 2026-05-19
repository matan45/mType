#include "TypeNameValidator.hpp"
#include "../../../types/TypeConversionUtils.hpp"

namespace vm::compiler::types
{
    bool isValidTypeName(const std::string& typeName,
                         const std::vector<std::string>& validGenericParams,
                         environment::Environment& env)
    {
        std::string baseTypeName = ::types::TypeConversionUtils::stripNullable(typeName);

        size_t bracketPos = baseTypeName.find('[');
        if (bracketPos != std::string::npos)
        {
            baseTypeName = baseTypeName.substr(0, bracketPos);
        }

        size_t anglePos = baseTypeName.find('<');
        if (anglePos != std::string::npos)
        {
            baseTypeName = baseTypeName.substr(0, anglePos);
        }

        if (baseTypeName == "int" || baseTypeName == "float" || baseTypeName == "string" ||
            baseTypeName == "bool" || baseTypeName == "void" || baseTypeName == "Array" ||
            baseTypeName == "object" || baseTypeName == "Promise")
        {
            return true;
        }

        for (const auto& genericParam : validGenericParams)
        {
            if (baseTypeName == genericParam)
            {
                return true;
            }
        }

        if (env.findClass(baseTypeName) != nullptr)
        {
            return true;
        }
        if (env.findInterface(baseTypeName) != nullptr)
        {
            return true;
        }

        return false;
    }
}
