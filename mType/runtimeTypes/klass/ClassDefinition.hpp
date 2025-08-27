#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

#include "ConstructorDefinition.hpp"
#include "FieldDefinition.hpp"
#include "MethodDefinition.hpp"
#include "../Definition.hpp"


namespace runtimeTypes::klass
{
    class ClassDefinition : public Definition
    {
    private:
        // Instance members
        std::unordered_map<std::string, std::shared_ptr<FieldDefinition>> instanceFields;
        std::unordered_map<std::string, std::shared_ptr<MethodDefinition>> instanceMethods;

        // Static members
        std::unordered_map<std::string, std::shared_ptr<FieldDefinition>> staticFields;
        std::unordered_map<std::string, std::shared_ptr<MethodDefinition>> staticMethods;

        // Constructors and destructor
        std::vector<std::shared_ptr<ConstructorDefinition>> constructors;

    public:
        explicit ClassDefinition(const std::string& n)
            : Definition(n)
        {
        }

        // Query methods
        bool hasField(const std::string& fieldName) const;
        bool hasMethod(const std::string& methodName) const;
        bool hasStaticField(const std::string& fieldName) const;
        bool hasStaticMethod(const std::string& methodName) const;
        ConstructorDefinition* findConstructor(size_t argCount) const;

        // Getter methods for AST node integration
        const std::string& getClassName() const;
        const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& getInstanceFields() const;
        const std::unordered_map<std::string, std::shared_ptr<MethodDefinition>>& getInstanceMethods() const;
        const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& getStaticFields() const;
        const std::unordered_map<std::string, std::shared_ptr<MethodDefinition>>& getStaticMethods() const;
        const std::vector<std::shared_ptr<ConstructorDefinition>>& getConstructors() const;

        // Setter/adder methods for AST node integration
        void addInstanceField(const std::string& name, std::shared_ptr<FieldDefinition> field);
        void addInstanceMethod(const std::string& name, std::shared_ptr<MethodDefinition> method);
        void addStaticField(const std::string& name, std::shared_ptr<FieldDefinition> field);
        void addStaticMethod(const std::string& name, std::shared_ptr<MethodDefinition> method);
        void addConstructor(std::shared_ptr<ConstructorDefinition> constructor);

        // Utility methods
        size_t getInstanceFieldCount() const;
        size_t getInstanceMethodCount() const;
        size_t getStaticFieldCount() const;
        size_t getStaticMethodCount() const;
        size_t getConstructorCount() const;
    };
}
