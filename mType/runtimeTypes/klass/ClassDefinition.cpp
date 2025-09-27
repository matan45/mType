#include "ClassDefinition.hpp"

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
        // Check direct interfaces first
        for (const auto& implementedInterface : implementedInterfaces) {
            if (implementedInterface == interfaceName) {
                return true;
            }
        }

        // Check transitive interface inheritance with depth protection
        std::unordered_set<std::string> visited;
        return implementsInterfaceTransitive(interfaceName, visited, 0);
    }

    bool ClassDefinition::implementsInterfaceTransitive(const std::string& interfaceName,
                                                       std::unordered_set<std::string>& visited,
                                                       int depth) const
    {
        // Depth protection - prevent stack overflow attacks
        if (depth > MAX_INTERFACE_DEPTH) {
            return false;
        }

        // Check all directly implemented interfaces for transitive inheritance
        for (const auto& implementedInterface : implementedInterfaces) {
            // Avoid infinite recursion (circular inheritance)
            if (visited.find(implementedInterface) != visited.end()) {
                continue;
            }

            visited.insert(implementedInterface);

            // TODO: For complete transitive resolution, we would need access to
            // the InterfaceRegistry to check if implementedInterface extends interfaceName
            // This is a simplified implementation that prevents the immediate vulnerability
            // Full implementation would require dependency injection of InterfaceRegistry

            visited.erase(implementedInterface);
        }

        return false;
    }

    bool ClassDefinition::isSubclassOf(const std::string& className) const
    {
        // TODO: Implement proper inheritance hierarchy checking
        // For now, return false since mType doesn't support class inheritance yet
        // This would need to traverse the inheritance chain when inheritance is implemented
        return false;
    }
}
