#include "TypeDescriptors.hpp"

namespace types
{
    // ExtendedTypeInfo
    ExtendedTypeInfo::ExtendedTypeInfo()
        : baseType(value::ValueType::VOID), isArray(false), arrayDimensions(0), isGeneric(false) {}

    ExtendedTypeInfo::ExtendedTypeInfo(value::ValueType type, const std::string& name)
        : baseType(type), typeName(name), isArray(false), arrayDimensions(0), isGeneric(false) {}

    ExtendedTypeInfo::ExtendedTypeInfo(value::ValueType type, const std::string& name, int dimensions)
        : baseType(type), typeName(name), isArray(dimensions > 0), arrayDimensions(dimensions), isGeneric(false) {}

    // ArrayTypeParser
    std::string ArrayTypeParser::createArrayTypeName(const std::string& elementType, int dimensions)
    {
        std::string result = elementType;
        for (int i = 0; i < dimensions; ++i)
        {
            result += "[]";
        }
        return result;
    }

    std::pair<std::string, int> ArrayTypeParser::parseArrayTypeName(const std::string& arrayTypeName)
    {
        std::string elementType = arrayTypeName;
        int dimensions = 0;

        size_t pos = arrayTypeName.length();
        while (pos >= 2 && arrayTypeName.substr(pos - 2, 2) == "[]")
        {
            dimensions++;
            pos -= 2;
        }

        if (dimensions > 0)
        {
            elementType = arrayTypeName.substr(0, pos);
        }

        return {elementType, dimensions};
    }

    // GenericInstantiationParser
    std::pair<std::string, std::vector<std::string>>
    GenericInstantiationParser::parse(const std::string& typeName)
    {
        size_t anglePos = typeName.find('<');
        if (anglePos == std::string::npos)
        {
            return {typeName, {}};
        }

        size_t endPos = typeName.rfind('>');
        if (endPos == std::string::npos || endPos <= anglePos)
        {
            return {typeName, {}};
        }

        std::string baseName = typeName.substr(0, anglePos);
        std::string typeArgsString = typeName.substr(anglePos + 1, endPos - anglePos - 1);

        std::vector<std::string> typeArgs;
        std::string currentArg;
        int depth = 0;

        for (char c : typeArgsString)
        {
            if (c == '<')
            {
                depth++;
                currentArg += c;
            }
            else if (c == '>')
            {
                depth--;
                currentArg += c;
            }
            else if (c == ',' && depth == 0)
            {
                size_t start = currentArg.find_first_not_of(" \t\n\r");
                if (start != std::string::npos)
                {
                    size_t end = currentArg.find_last_not_of(" \t\n\r");
                    typeArgs.push_back(currentArg.substr(start, end - start + 1));
                }
                currentArg.clear();
            }
            else
            {
                currentArg += c;
            }
        }

        size_t start = currentArg.find_first_not_of(" \t\n\r");
        if (start != std::string::npos)
        {
            size_t end = currentArg.find_last_not_of(" \t\n\r");
            typeArgs.push_back(currentArg.substr(start, end - start + 1));
        }

        return {baseName, typeArgs};
    }

    // InheritanceChainTraverser
    InheritanceChainTraverser::InheritanceChainTraverser(
        const std::unordered_map<std::string, std::string>& inheritance)
        : classInheritance(inheritance) {}

    bool InheritanceChainTraverser::traverse(const std::string& childType,
                                             const std::string& targetParent) const
    {
        std::string current = childType;
        std::unordered_set<std::string> visited;
        int depth = 0;

        while (depth < MAX_DEPTH)
        {
            auto it = classInheritance.find(current);
            if (it == classInheritance.end())
            {
                return false;
            }

            const std::string& parent = it->second;

            if (visited.find(parent) != visited.end())
            {
                return false;
            }
            visited.insert(parent);

            if (parent == targetParent)
            {
                return true;
            }

            current = parent;
            depth++;
        }

        return false;
    }

    int InheritanceChainTraverser::calculateDistance(const std::string& childType,
                                                     const std::string& targetParent) const
    {
        if (childType == targetParent)
        {
            return 0;
        }

        std::string current = childType;
        std::unordered_set<std::string> visited;
        int distance = 0;

        while (distance < MAX_DEPTH)
        {
            auto it = classInheritance.find(current);
            if (it == classInheritance.end())
            {
                return -1;
            }

            const std::string& parent = it->second;

            if (visited.find(parent) != visited.end())
            {
                return -1;
            }
            visited.insert(parent);

            distance++;

            if (parent == targetParent)
            {
                return distance;
            }

            current = parent;
        }

        return -1;
    }
}
