#include "ObjectInstance.hpp"
#include <algorithm>
#include <vector>

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

    std::string ObjectInstance::getContentHash() const
    {
        std::string hash = classDefinition->getName() + ":";
        
        // Get all instance fields in sorted order for consistent hashing
        const auto& fields = classDefinition->getInstanceFields();
        std::vector<std::string> fieldNames;
        for (const auto& pair : fields) {
            if (!pair.second->isStatic()) {  // Only instance fields
                fieldNames.push_back(pair.first);
            }
        }
        std::sort(fieldNames.begin(), fieldNames.end());
        
        // Build hash from field values
        for (const std::string& fieldName : fieldNames) {
            Value fieldValue = getFieldValue(fieldName);
            hash += fieldName + "=";
            
            // Convert field value to string
            std::visit([&hash](const auto& v) {
                if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>) {
                    hash += std::to_string(v);
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, float>) {
                    hash += std::to_string(v);
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
                    hash += v ? "true" : "false";
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                    hash += v;
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, nullptr_t>) {
                    hash += "null";
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
                    hash += "void";
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>) {
                    // Recursive content hashing for nested objects
                    hash += v ? v->getContentHash() : "null_obj";
                } else {
                    // For collections and other complex types, use reference-based hashing
                    // This is acceptable since deep content comparison for collections would be expensive
                    hash += "ref_" + std::to_string(reinterpret_cast<uintptr_t>(&v));
                }
            }, fieldValue);
            hash += ";";
        }
        
        return hash;
    }
}