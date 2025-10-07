#pragma once
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

namespace vm::compiler::types
{
    /**
     * Resolves generic type parameters and performs type substitution
     * Handles parsing and transformation of generic type names
     */
    class GenericTypeResolver
    {
    public:
        GenericTypeResolver() = default;
        ~GenericTypeResolver() = default;

        // Parse generic interface/class name into base name and type parameters
        // Example: "Comparable<String>" -> ("Comparable", ["String"])
        std::pair<std::string, std::vector<std::string>> parseGenericTypeName(
            const std::string& typeName
        ) const;

        // Resolve generic type with substitutions
        // Example: resolveGenericType("T", {{"T", "String"}}) -> "String"
        std::string resolveGenericType(
            const std::string& typeName,
            const std::unordered_map<std::string, std::string>& substitutions
        ) const;

        // Extract base type name (without generic parameters)
        // Example: "List<Int>" -> "List"
        std::string extractBaseTypeName(const std::string& typeName) const;

        // Extract generic type arguments
        // Example: "Map<String,Int>" -> ["String", "Int"]
        std::vector<std::string> extractTypeArguments(const std::string& typeName) const;
    };
}
