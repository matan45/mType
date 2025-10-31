#include "TypeRegistry.hpp"
#include "../circularDependency/TrueCyclicException.hpp"
#include "../circularDependency/DepthLimitException.hpp"

namespace parser
{
    TypeRegistry::TypeRegistry()
        : dependencyDetector(std::make_unique<circularDependency::CircularDependencyDetector>())
    {
    }

    std::string TypeRegistry::extractBaseName(const std::string& typeName) const noexcept
    {
        size_t genericStart = typeName.find('<');
        if (genericStart != std::string::npos)
        {
            return typeName.substr(0, genericStart);
        }
        return typeName;
    }

    std::vector<std::string> TypeRegistry::buildClassInheritanceChain(
        const std::string& childClass,
        const std::string& parentClass) const
    {
        std::vector<std::string> chain;
        chain.push_back(childClass);

        // Walk up the parent chain
        std::string current = parentClass;
        while (!current.empty())
        {
            chain.push_back(current);

            auto it = classParents.find(current);
            if (it != classParents.end())
            {
                current = it->second;
            }
            else
            {
                break;
            }
        }

        return chain;
    }

    std::vector<std::string> TypeRegistry::buildInterfaceInheritanceChain(
        const std::string& childInterface,
        const std::vector<std::string>& parentInterfaces) const
    {
        std::vector<std::string> chain;
        chain.push_back(childInterface);

        // Use BFS to build full chain including all parent paths
        std::vector<std::string> toVisit = parentInterfaces;
        std::unordered_set<std::string> visited;
        visited.insert(childInterface);

        while (!toVisit.empty())
        {
            std::string current = toVisit.back();
            toVisit.pop_back();

            if (visited.count(current) > 0)
            {
                continue; // Skip already visited
            }

            visited.insert(current);
            chain.push_back(current);

            // Add parents of current interface
            auto it = interfaceParents.find(current);
            if (it != interfaceParents.end())
            {
                for (const auto& parent : it->second)
                {
                    if (visited.count(parent) == 0)
                    {
                        toVisit.push_back(parent);
                    }
                }
            }
        }

        return chain;
    }

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
        typeDeclarationLocations.clear();

        // Reset dependency detector for fresh validation
        if (dependencyDetector)
        {
            dependencyDetector->resetAll();
        }
    }

    bool TypeRegistry::isClassDeclared(const std::string& className) const noexcept
    {
        return declaredClasses.count(className) > 0;
    }

    bool TypeRegistry::isInterfaceDeclared(const std::string& interfaceName) const noexcept
    {
        return declaredInterfaces.count(interfaceName) > 0;
    }

    void TypeRegistry::registerClass(const std::string& className, bool isFinal,
                                     const errors::SourceLocation& location) noexcept
    {
        declaredClasses.insert(className);
        declaredTypeNames.insert(className);
        typeDeclarationLocations[className] = location;
        if (isFinal)
        {
            finalClasses.insert(className);
        }
    }

    void TypeRegistry::registerInterface(const std::string& interfaceName, bool isFinal,
                                         const errors::SourceLocation& location) noexcept
    {
        declaredInterfaces.insert(interfaceName);
        declaredTypeNames.insert(interfaceName);
        typeDeclarationLocations[interfaceName] = location;
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
        // Extract base name without generic parameters
        std::string baseParent = extractBaseName(parentClass);

        // Check for immediate self-inheritance
        if (childClass == baseParent)
        {
            return false;
        }

        // Build the full inheritance chain for validation
        std::vector<std::string> chain = buildClassInheritanceChain(childClass, baseParent);

        // Use CircularDependencyDetector to validate the chain
        try
        {
            dependencyDetector->validateChain(
                circularDependency::DependencyType::CLASS_INHERITANCE,
                chain,
                "Class: " + childClass
            );

            // No cycle found, register the relationship
            classParents[childClass] = baseParent;
            return true;
        }
        catch (const circularDependency::TrueCyclicException&)
        {
            // Cycle detected
            return false;
        }
        catch (const circularDependency::DepthLimitException&)
        {
            // Depth limit exceeded (also considered invalid)
            return false;
        }
    }

    bool TypeRegistry::registerInterfaceInheritance(const std::string& childInterface,
                                                    const std::vector<std::string>& parentInterfaces) noexcept
    {
        // Extract base names without generic parameters
        std::vector<std::string> baseParents;
        for (const auto& parent : parentInterfaces)
        {
            std::string baseParent = extractBaseName(parent);

            // Check for immediate self-inheritance
            if (childInterface == baseParent)
            {
                return false;
            }

            baseParents.push_back(baseParent);
        }

        // Build the full inheritance chain for validation
        std::vector<std::string> chain = buildInterfaceInheritanceChain(childInterface, baseParents);

        // Use CircularDependencyDetector to validate the chain
        try
        {
            dependencyDetector->validateChain(
                circularDependency::DependencyType::INTERFACE_INHERITANCE,
                chain,
                "Interface: " + childInterface
            );

            // No cycle found, register the relationships
            interfaceParents[childInterface] = baseParents;
            return true;
        }
        catch (const circularDependency::TrueCyclicException&)
        {
            // Cycle detected
            return false;
        }
        catch (const circularDependency::DepthLimitException&)
        {
            // Depth limit exceeded (also considered invalid)
            return false;
        }
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
        chain.push_back(current); // Add final parent

        return chain;
    }

    bool TypeRegistry::isFunctionDeclared(const std::string& functionName) const noexcept
    {
        return declaredFunctionNames.count(functionName) > 0;
    }

    void TypeRegistry::registerFunctionName(const std::string& functionName,
                                           const errors::SourceLocation& location) noexcept
    {
        declaredFunctionNames.insert(functionName);
        functionDeclarationLocations[functionName] = location;
    }

    void TypeRegistry::clearDeclaredFunctions() noexcept
    {
        declaredFunctionNames.clear();
        functionDeclarationLocations.clear();
    }

    errors::SourceLocation TypeRegistry::getTypeDeclarationLocation(const std::string& typeName) const noexcept
    {
        auto it = typeDeclarationLocations.find(typeName);
        if (it != typeDeclarationLocations.end())
        {
            return it->second;
        }
        return errors::SourceLocation(); // Return default location if not found
    }

    errors::SourceLocation TypeRegistry::getFunctionDeclarationLocation(const std::string& functionName) const noexcept
    {
        auto it = functionDeclarationLocations.find(functionName);
        if (it != functionDeclarationLocations.end())
        {
            return it->second;
        }
        return errors::SourceLocation(); // Return default location if not found
    }
}
