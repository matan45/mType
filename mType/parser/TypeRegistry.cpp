#include "TypeRegistry.hpp"

namespace parser
{
    bool TypeRegistry::isTypeDeclared(const std::string& typeName) const noexcept
    {
        return declaredTypeNames.count(typeName) > 0;
    }

    void TypeRegistry::registerTypeName(const std::string& typeName) noexcept
    {
        declaredTypeNames.insert(typeName);
    }

    void TypeRegistry::clearDeclaredTypes() noexcept
    {
        declaredTypeNames.clear();
        declaredClasses.clear();
        declaredInterfaces.clear();
        finalClasses.clear();
        finalInterfaces.clear();
        classParents.clear();
        interfaceParents.clear();
    }

    bool TypeRegistry::isClassDeclared(const std::string& className) const noexcept
    {
        return declaredClasses.count(className) > 0;
    }

    bool TypeRegistry::isInterfaceDeclared(const std::string& interfaceName) const noexcept
    {
        return declaredInterfaces.count(interfaceName) > 0;
    }

    void TypeRegistry::registerClass(const std::string& className, bool isFinal) noexcept
    {
        declaredClasses.insert(className);
        declaredTypeNames.insert(className);
        if (isFinal)
        {
            finalClasses.insert(className);
        }
    }

    void TypeRegistry::registerInterface(const std::string& interfaceName, bool isFinal) noexcept
    {
        declaredInterfaces.insert(interfaceName);
        declaredTypeNames.insert(interfaceName);
        if (isFinal)
        {
            finalInterfaces.insert(interfaceName);
        }
    }

    bool TypeRegistry::isClassFinal(const std::string& className) const noexcept
    {
        return finalClasses.count(className) > 0;
    }

    bool TypeRegistry::isInterfaceFinal(const std::string& interfaceName) const noexcept
    {
        return finalInterfaces.count(interfaceName) > 0;
    }

    bool TypeRegistry::registerClassInheritance(const std::string& childClass, const std::string& parentClass) noexcept
    {
        // Extract base names without generic parameters
        std::string baseParent = parentClass;
        size_t genericStart = parentClass.find('<');
        if (genericStart != std::string::npos)
        {
            baseParent = parentClass.substr(0, genericStart);
        }

        // Check for immediate self-inheritance
        if (childClass == baseParent)
        {
            return false;
        }

        // Check for circular inheritance by traversing parent chain
        std::string current = baseParent;
        std::unordered_set<std::string> visited;
        visited.insert(childClass);

        while (classParents.count(current) > 0)
        {
            if (visited.count(current) > 0)
            {
                // Cycle detected
                return false;
            }
            visited.insert(current);
            current = classParents[current];

            // Extract base name from parent
            genericStart = current.find('<');
            if (genericStart != std::string::npos)
            {
                current = current.substr(0, genericStart);
            }
        }

        // No cycle found, register the relationship
        classParents[childClass] = baseParent;
        return true;
    }

    bool TypeRegistry::registerInterfaceInheritance(const std::string& childInterface,
                                                    const std::vector<std::string>& parentInterfaces) noexcept
    {
        // Extract base names without generic parameters
        std::vector<std::string> baseParents;
        for (const auto& parent : parentInterfaces)
        {
            std::string baseParent = parent;
            size_t genericStart = parent.find('<');
            if (genericStart != std::string::npos)
            {
                baseParent = parent.substr(0, genericStart);
            }

            // Check for immediate self-inheritance
            if (childInterface == baseParent)
            {
                return false;
            }

            baseParents.push_back(baseParent);
        }

        // Check for circular inheritance using DFS
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recursionStack;

        auto hasCycle = [this, &visited, &recursionStack](const std::string& interface, auto& hasCycleRef) -> bool
        {
            if (recursionStack.count(interface) > 0)
            {
                // Found cycle
                return true;
            }

            if (visited.count(interface) > 0)
            {
                // Already visited this path, no cycle
                return false;
            }

            visited.insert(interface);
            recursionStack.insert(interface);

            // Check all parents of this interface
            if (interfaceParents.count(interface) > 0)
            {
                for (const auto& parent : interfaceParents.at(interface))
                {
                    if (hasCycleRef(parent, hasCycleRef))
                    {
                        return true;
                    }
                }
            }

            recursionStack.erase(interface);
            return false;
        };

        // Check if adding these parents would create a cycle
        for (const auto& baseParent : baseParents)
        {
            visited.clear();
            recursionStack.clear();
            recursionStack.insert(childInterface);

            if (hasCycle(baseParent, hasCycle))
            {
                return false;
            }
        }

        // No cycle found, register the relationships
        interfaceParents[childInterface] = baseParents;
        return true;
    }

    std::vector<std::string> TypeRegistry::getClassInheritanceChain(const std::string& className) const noexcept
    {
        std::vector<std::string> chain;
        std::string current = className;

        while (classParents.count(current) > 0)
        {
            chain.push_back(current);
            current = classParents.at(current);
        }
        chain.push_back(current);  // Add final parent

        return chain;
    }

    bool TypeRegistry::isFunctionDeclared(const std::string& functionName) const noexcept
    {
        return declaredFunctionNames.count(functionName) > 0;
    }

    void TypeRegistry::registerFunctionName(const std::string& functionName) noexcept
    {
        declaredFunctionNames.insert(functionName);
    }

    void TypeRegistry::clearDeclaredFunctions() noexcept
    {
        declaredFunctionNames.clear();
    }
}
