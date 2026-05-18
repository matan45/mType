#pragma once

#include "../value/ValueType.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace types
{
    /**
     * Enhanced type information supporting arrays, generics, and custom types.
     */
    struct ExtendedTypeInfo
    {
        value::ValueType baseType;
        std::string typeName;
        bool isArray;
        int arrayDimensions;
        bool isGeneric;
        std::vector<std::string> genericParameters;
        std::string fullyQualifiedName;

        ExtendedTypeInfo();
        ExtendedTypeInfo(value::ValueType type, const std::string& name = "");
        ExtendedTypeInfo(value::ValueType type, const std::string& name, int dimensions);
    };

    /**
     * Helper class to handle array type name creation and parsing.
     */
    class ArrayTypeParser
    {
    public:
        static std::string createArrayTypeName(const std::string& elementType, int dimensions);
        static std::pair<std::string, int> parseArrayTypeName(const std::string& arrayTypeName);
    };

    /**
     * Helper class to parse generic type instantiations (e.g., "List<String>").
     */
    class GenericInstantiationParser
    {
    public:
        static std::pair<std::string, std::vector<std::string>> parse(const std::string& typeName);
    };

    /**
     * Helper class to traverse string-keyed inheritance chains with cycle detection.
     */
    class InheritanceChainTraverser
    {
    private:
        const std::unordered_map<std::string, std::string>& classInheritance;
        static const int MAX_DEPTH = 20;

    public:
        explicit InheritanceChainTraverser(const std::unordered_map<std::string, std::string>& inheritance);

        bool traverse(const std::string& childType, const std::string& targetParent) const;
        int calculateDistance(const std::string& childType, const std::string& targetParent) const;
    };
}
