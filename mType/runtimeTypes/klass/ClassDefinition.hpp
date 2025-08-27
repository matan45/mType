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

        bool hasField(const std::string& fieldName) const;
        bool hasMethod(const std::string& methodName) const;
        ConstructorDefinition* findConstructor(size_t argCount) const;
    };
}
