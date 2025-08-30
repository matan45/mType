#include "ObjectInstance.hpp"

namespace runtimeTypes::klass
{
    std::shared_ptr<FieldDefinition> ObjectInstance::getField(const std::string& fieldName) const
    {
        return classDefinition->getField(fieldName);
    }

    Value ObjectInstance::getFieldValue(const std::string& fieldName) const
    {
        auto field = getField(fieldName);
        if (field) {
            // For static fields, get value from the field definition
            if (field->isStatic()) {
                return field->getValue();
            }
            // For instance fields, get value from this instance's storage
            auto it = fieldValues.find(fieldName);
            if (it != fieldValues.end()) {
                return it->second;
            }
            // Return default value if not set
            return field->getValue(); // This will be the initial value from class definition
        }
        return std::monostate{};
    }

    std::shared_ptr<ClassDefinition> ObjectInstance::getClassDefinition() const
    {
        return classDefinition;
    }

    void ObjectInstance::setField(const std::string& fieldName, const Value& value)
    {
        auto field = getField(fieldName);
        if (field) {
            // For static fields, set value in the field definition (shared across all instances)
            if (field->isStatic()) {
                field->setValue(value);
            } else {
                // For instance fields, set value in this instance's storage
                fieldValues[fieldName] = value;
            }
        }
    }

    bool ObjectInstance::isInstanceOf(const std::string& className) const
    {
        return classDefinition && classDefinition->getName() == className;
    }

    std::string ObjectInstance::getTypeName() const
    {
        return classDefinition ? classDefinition->getName() : "unknown";
    }

    Value ObjectInstance::callMethod(const std::string& methodName, const std::vector<Value>& args)
    {
        auto method = classDefinition->getMethod(methodName);
        if (!method) {
            return std::monostate{};
        }
        
        // Method execution would need evaluator context
        // This is a simplified implementation
        return std::monostate{};
    }
}