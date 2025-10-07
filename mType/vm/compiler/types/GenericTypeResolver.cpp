#include "GenericTypeResolver.hpp"
#include <algorithm>

namespace vm::compiler::types
{
    std::pair<std::string, std::vector<std::string>> GenericTypeResolver::parseGenericTypeName(
        const std::string& typeName
    ) const
    {
        std::string baseName = extractBaseTypeName(typeName);
        std::vector<std::string> typeArgs = extractTypeArguments(typeName);
        return {baseName, typeArgs};
    }

    std::string GenericTypeResolver::resolveGenericType(
        const std::string& typeName,
        const std::unordered_map<std::string, std::string>& substitutions
    ) const
    {
        // Check if this is a generic parameter that needs substitution
        auto it = substitutions.find(typeName);
        if (it != substitutions.end()) {
            return it->second;
        }

        // If the type has generic parameters, recursively resolve them
        size_t genericStart = typeName.find('<');
        if (genericStart == std::string::npos) {
            return typeName;  // No generic parameters, return as-is
        }

        // Extract base name and type arguments
        std::string baseName = typeName.substr(0, genericStart);
        std::vector<std::string> typeArgs = extractTypeArguments(typeName);

        // Resolve each type argument
        std::vector<std::string> resolvedArgs;
        for (const auto& arg : typeArgs) {
            resolvedArgs.push_back(resolveGenericType(arg, substitutions));
        }

        // Reconstruct the type name with resolved arguments
        std::string result = baseName + "<";
        for (size_t i = 0; i < resolvedArgs.size(); ++i) {
            if (i > 0) result += ",";
            result += resolvedArgs[i];
        }
        result += ">";

        return result;
    }

    std::string GenericTypeResolver::extractBaseTypeName(const std::string& typeName) const
    {
        size_t genericStart = typeName.find('<');
        if (genericStart != std::string::npos) {
            return typeName.substr(0, genericStart);
        }
        return typeName;
    }

    std::vector<std::string> GenericTypeResolver::extractTypeArguments(const std::string& typeName) const
    {
        std::vector<std::string> typeArguments;

        size_t genericStart = typeName.find('<');
        if (genericStart == std::string::npos) {
            return typeArguments;  // No generic parameters
        }

        size_t genericEnd = typeName.rfind('>');
        if (genericEnd == std::string::npos || genericEnd <= genericStart) {
            return typeArguments;  // Invalid format
        }

        std::string typeArgsStr = typeName.substr(genericStart + 1, genericEnd - genericStart - 1);

        // Parse individual type arguments, respecting nested generics
        size_t start = 0;
        size_t depth = 0;
        for (size_t i = 0; i < typeArgsStr.length(); ++i) {
            if (typeArgsStr[i] == '<') {
                depth++;
            } else if (typeArgsStr[i] == '>') {
                depth--;
            } else if (typeArgsStr[i] == ',' && depth == 0) {
                std::string arg = typeArgsStr.substr(start, i - start);
                // Trim whitespace
                arg.erase(0, arg.find_first_not_of(" \t"));
                arg.erase(arg.find_last_not_of(" \t") + 1);
                typeArguments.push_back(arg);
                start = i + 1;
            }
        }

        // Add last argument
        std::string arg = typeArgsStr.substr(start);
        arg.erase(0, arg.find_first_not_of(" \t"));
        arg.erase(arg.find_last_not_of(" \t") + 1);
        if (!arg.empty()) {
            typeArguments.push_back(arg);
        }

        return typeArguments;
    }
}
