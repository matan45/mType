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
}
