#include "ClassDefinition.hpp"
#include "InterfaceRegistry.hpp"
#include "InterfaceDefinition.hpp"
#include <iostream>

namespace runtimeTypes::klass
{
    // Query methods
    bool ClassDefinition::hasField(const std::string& fieldName) const
    {
        return instanceFields.find(fieldName) != instanceFields.end();
    }

    bool ClassDefinition::hasMethod(const std::string& methodName) const
    {
        return instanceMethods.find(methodName) != instanceMethods.end();
    }

    bool ClassDefinition::hasStaticField(const std::string& fieldName) const
    {
        return staticFields.find(fieldName) != staticFields.end();
    }

    bool ClassDefinition::hasStaticMethod(const std::string& methodName) const
    {
        return staticMethods.find(methodName) != staticMethods.end();
    }

    ConstructorDefinition* ClassDefinition::findConstructor(size_t argCount) const
    {
        for (const auto& constructor : constructors) {
            if (constructor->getParameterCount() == argCount) {
                return constructor.get();
            }
        }
        return nullptr;
    }

    // Getter methods for AST node integration
    const std::string& ClassDefinition::getClassName() const
    {
        return getName(); // From base Definition class
    }

    const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& ClassDefinition::getInstanceFields() const
    {
        return instanceFields;
    }

    const std::unordered_map<std::string, std::shared_ptr<MethodDefinition>>& ClassDefinition::getInstanceMethods() const
    {
        return instanceMethods;
    }

    const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& ClassDefinition::getStaticFields() const
    {
        return staticFields;
    }

    const std::unordered_map<std::string, std::shared_ptr<MethodDefinition>>& ClassDefinition::getStaticMethods() const
    {
        return staticMethods;
    }

    const std::vector<std::shared_ptr<ConstructorDefinition>>& ClassDefinition::getConstructors() const
    {
        return constructors;
    }

    // Setter/adder methods for AST node integration
    void ClassDefinition::addInstanceField(const std::string& name, std::shared_ptr<FieldDefinition> field)
    {
        instanceFields[name] = field;
    }

    void ClassDefinition::addInstanceMethod(const std::string& name, std::shared_ptr<MethodDefinition> method)
    {
        instanceMethods[name] = method;
    }

    void ClassDefinition::addStaticField(const std::string& name, std::shared_ptr<FieldDefinition> field)
    {
        staticFields[name] = field;
    }

    void ClassDefinition::addStaticMethod(const std::string& name, std::shared_ptr<MethodDefinition> method)
    {
        staticMethods[name] = method;
    }

    void ClassDefinition::addConstructor(std::shared_ptr<ConstructorDefinition> constructor)
    {
        constructors.push_back(constructor);
    }

    // Utility methods
    size_t ClassDefinition::getInstanceFieldCount() const
    {
        return instanceFields.size();
    }

    size_t ClassDefinition::getInstanceMethodCount() const
    {
        return instanceMethods.size();
    }

    size_t ClassDefinition::getStaticFieldCount() const
    {
        return staticFields.size();
    }

    size_t ClassDefinition::getStaticMethodCount() const
    {
        return staticMethods.size();
    }

    size_t ClassDefinition::getConstructorCount() const
    {
        return constructors.size();
    }

    // Additional compatibility methods implementation
    void ClassDefinition::addField(std::shared_ptr<FieldDefinition> field)
    {
        if (field->isStatic()) {
            addStaticField(field->getName(), field);
        } else {
            addInstanceField(field->getName(), field);
        }
    }

    void ClassDefinition::addMethod(std::shared_ptr<MethodDefinition> method)
    {
        if (method->isStatic()) {
            addStaticMethod(method->getName(), method);
        } else {
            addInstanceMethod(method->getName(), method);
        }
    }

    std::shared_ptr<FieldDefinition> ClassDefinition::getField(const std::string& fieldName) const
    {
        auto it = instanceFields.find(fieldName);
        if (it != instanceFields.end()) {
            return it->second;
        }

        auto staticIt = staticFields.find(fieldName);
        if (staticIt != staticFields.end()) {
            return staticIt->second;
        }

        return nullptr;
    }

    std::shared_ptr<FieldDefinition> ClassDefinition::getFieldInHierarchy(const std::string& fieldName) const
    {
        // First, check in this class
        auto field = getField(fieldName);
        if (field) {
            return field;
        }

        // Then check in parent class hierarchy
        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            field = current->getField(fieldName);
            if (field) {
                return field;
            }
            current = current->parentClass.lock();
            depth++;
        }

        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::getMethod(const std::string& methodName) const
    {
        auto it = instanceMethods.find(methodName);
        if (it != instanceMethods.end()) {
            return it->second;
        }
        
        auto staticIt = staticMethods.find(methodName);
        if (staticIt != staticMethods.end()) {
            return staticIt->second;
        }
        
        return nullptr;
    }

    std::shared_ptr<ConstructorDefinition> ClassDefinition::getConstructor() const
    {
        return constructors.empty() ? nullptr : constructors[0];
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findMethod(const std::string& methodName, size_t argCount) const
    {
        auto method = getMethod(methodName);
        if (method && method->getParameters().size() == argCount) {
            return method;
        }
        return nullptr;
    }

    // NEW: Generic-related method implementations
    std::string ClassDefinition::getGenericClassName() const
    {
        if (!isGeneric()) {
            return getName();
        }

        std::string result = getName() + "<";
        for (size_t i = 0; i < genericParameters.size(); ++i) {
            if (i > 0) result += ", ";
            result += genericParameters[i].name;
        }
        result += ">";
        return result;
    }

    bool ClassDefinition::hasGenericParameter(const std::string& paramName) const
    {
        for (const auto& param : genericParameters) {
            if (param.name == paramName) {
                return true;
            }
        }
        return false;
    }

    bool ClassDefinition::implementsInterface(const std::string& interfaceName) const
    {
        // Direct interface checking only - for transitive checks, use the registry-based version
        for (const auto& implementedInterface : implementedInterfaces) {
            // Extract base name from generic interface (e.g., "CacheStore<String, Int, String>" -> "CacheStore")
            std::string baseImplementedName = implementedInterface;
            size_t anglePos = implementedInterface.find('<');
            if (anglePos != std::string::npos) {
                baseImplementedName = implementedInterface.substr(0, anglePos);
            }

            if (baseImplementedName == interfaceName) {
                return true;
            }
        }

        // Cannot perform transitive interface resolution without InterfaceRegistry
        return false;
    }

    bool ClassDefinition::implementsInterface(const std::string& interfaceName, std::shared_ptr<InterfaceRegistry> registry) const
    {
        // Check direct interfaces first
        for (const auto& implementedInterface : implementedInterfaces) {
            // Extract base name from generic interface (e.g., "CacheStore<String, Int, String>" -> "CacheStore")
            std::string baseImplementedName = implementedInterface;
            size_t anglePos = implementedInterface.find('<');
            if (anglePos != std::string::npos) {
                baseImplementedName = implementedInterface.substr(0, anglePos);
            }

            if (baseImplementedName == interfaceName) {
                return true;
            }
        }

        // Check transitive interface inheritance with depth protection and registry access
        std::unordered_set<std::string> visited;
        return implementsInterfaceTransitive(interfaceName, visited, 0, registry);
    }

    bool ClassDefinition::implementsInterfaceTransitive(const std::string& interfaceName,
                                                       std::unordered_set<std::string>& visited,
                                                       int depth,
                                                       std::shared_ptr<InterfaceRegistry> registry) const
    {
        // Depth protection - prevent stack overflow attacks
        if (depth > MAX_INTERFACE_DEPTH) {
            return false;
        }

        if (!registry) {
            return false;
        }

        // Check all directly implemented interfaces for transitive inheritance
        for (const auto& implementedInterface : implementedInterfaces) {
            // Extract base interface name (e.g., "CacheStore<String, Int, String>" -> "CacheStore")
            std::string baseImplementedName = implementedInterface;
            size_t anglePos = implementedInterface.find('<');
            if (anglePos != std::string::npos) {
                baseImplementedName = implementedInterface.substr(0, anglePos);
            }

            // Avoid infinite recursion (circular inheritance)
            if (visited.find(baseImplementedName) != visited.end()) {
                continue;
            }

            visited.insert(baseImplementedName);

            // Look up the interface definition in the registry
            auto interfaceDef = registry->findInterface(baseImplementedName);
            if (interfaceDef) {
                // Check what interfaces this interface extends
                const auto& extendedInterfaces = interfaceDef->getExtendedInterfaces();
                for (const auto& extendedInterface : extendedInterfaces) {
                    // Extract base name from extended interface
                    std::string baseExtendedName = extendedInterface;
                    size_t extAnglePos = extendedInterface.find('<');
                    if (extAnglePos != std::string::npos) {
                        baseExtendedName = extendedInterface.substr(0, extAnglePos);
                    }

                    // Check if this extended interface matches what we're looking for
                    if (baseExtendedName == interfaceName) {
                        visited.erase(baseImplementedName);
                        return true;
                    }

                    // Recursively check if the extended interface implements our target
                    // We need to check if the extended interface (e.g., "Repository") extends our target (e.g., "Hashable")
                    auto extendedInterfaceDef = registry->findInterface(baseExtendedName);
                    if (extendedInterfaceDef) {
                        // Check if the extended interface directly or transitively extends our target
                        const auto& furtherExtended = extendedInterfaceDef->getExtendedInterfaces();
                        for (const auto& furtherInterface : furtherExtended) {
                            std::string furtherBaseName = furtherInterface;
                            size_t furtherAnglePos = furtherInterface.find('<');
                            if (furtherAnglePos != std::string::npos) {
                                furtherBaseName = furtherInterface.substr(0, furtherAnglePos);
                            }

                            if (furtherBaseName == interfaceName) {
                                visited.erase(baseImplementedName);
                                return true;
                            }
                        }
                    }
                }
            }

            visited.erase(baseImplementedName);
        }

        return false;
    }

    bool ClassDefinition::isSubclassOf(const std::string& className) const
    {
        // Check direct parent
        if (parentClassName == className) {
            return true;
        }

        // Traverse inheritance chain with depth protection
        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            if (current->getName() == className) {
                return true;
            }
            current = current->parentClass.lock();
            depth++;
        }

        return false;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findMethodInHierarchy(const std::string& methodName, size_t argCount) const
    {
        // First, check in this class
        auto method = findMethod(methodName, argCount);
        if (method) {
            return method;
        }

        // Then check in parent class hierarchy
        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            method = current->findMethod(methodName, argCount);
            if (method) {
                return method;
            }
            current = current->parentClass.lock();
            depth++;
        }

        return nullptr;
    }

    std::vector<std::shared_ptr<ClassDefinition>> ClassDefinition::getInheritanceChain() const
    {
        std::vector<std::shared_ptr<ClassDefinition>> chain;

        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            chain.push_back(current);
            current = current->parentClass.lock();
            depth++;
        }

        return chain;
    }
}
