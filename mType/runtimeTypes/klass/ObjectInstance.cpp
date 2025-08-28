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
            return field->getValue();
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
            field->setValue(value);
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