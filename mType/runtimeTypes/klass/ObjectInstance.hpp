#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "../../value/ValueType.hpp"
#include "ClassDefinition.hpp"
#include "FieldDefinition.hpp"

namespace runtimeTypes::klass
{
    using namespace value;
    class ObjectInstance : public std::enable_shared_from_this<ObjectInstance>
    {
    private:
        std::shared_ptr<ClassDefinition> classDefinition;
        std::unordered_map<std::string, Value> fieldValues;
        std::weak_ptr<ObjectInstance> parent; // For nested objects
    public :
        ObjectInstance(std::shared_ptr<ClassDefinition> classDef)
            : classDefinition(classDef)
        {
        }

        std::shared_ptr<FieldDefinition> getField(const std::string& fieldName) const;
        Value getFieldValue(const std::string& fieldName) const;
        std::shared_ptr<ClassDefinition> getClassDefinition() const;
        void setField(const std::string& fieldName, const Value& value);
        // Type checking
        bool isInstanceOf(const std::string& className) const;
        std::string getTypeName() const;
        // Call method on this instance
        Value callMethod(const std::string& methodName,
                         const std::vector<Value>& args);
        
        // Generate content-based hash for Set/Map operations
        std::string getContentHash() const;
    };
}
