// ObjectInstance stub for language server - LSP doesn't need runtime object instances
#include "../../../mType/runtimeTypes/klass/ObjectInstance.hpp"

namespace runtimeTypes::klass
{
    void ObjectInstance::registerWithGC() {
        // No-op for LSP
    }

    std::shared_ptr<FieldDefinition> ObjectInstance::getField(const std::string& fieldName) const {
        return classDefinition ? classDefinition->getFieldInHierarchy(fieldName) : nullptr;
    }

    Value ObjectInstance::getFieldValue(const std::string& fieldName) const {
        auto it = fieldValues.find(fieldName);
        if (it != fieldValues.end()) {
            return it->second;
        }
        return std::monostate{};
    }

    std::shared_ptr<ClassDefinition> ObjectInstance::getClassDefinition() const {
        return classDefinition;
    }

    void ObjectInstance::setField(const std::string& fieldName, const Value& value) {
        fieldValues[fieldName] = value;
    }

    bool ObjectInstance::isInstanceOf(const std::string& className) const {
        if (!classDefinition) return false;
        if (classDefinition->getName() == className) return true;
        return classDefinition->isSubclassOf(className);
    }

    std::string ObjectInstance::getTypeName() const {
        return classDefinition ? classDefinition->getName() : "unknown";
    }

    std::string ObjectInstance::getFullTypeName() const {
        if (!classDefinition) return "unknown";
        return classDefinition->getName();
    }

    std::string ObjectInstance::getContentHash() const {
        return classDefinition ? classDefinition->getName() : "unknown";
    }

    bool ObjectInstance::compareFieldValues(const Value& thisValue, const Value& otherValue) {
        return false; // Not needed for LSP
    }

    bool ObjectInstance::contentEquals(const ObjectInstance& other) const {
        return false; // Not needed for LSP
    }

    void ObjectInstance::setGenericTypeBinding(const std::string& parameter, const std::string& concreteType) {
        genericTypeBindings[parameter] = concreteType;
    }

    std::string ObjectInstance::resolveGenericType(const std::string& typeName) const {
        auto it = genericTypeBindings.find(typeName);
        return it != genericTypeBindings.end() ? it->second : typeName;
    }

    const std::unordered_map<std::string, std::string>& ObjectInstance::getGenericTypeBindings() const {
        return genericTypeBindings;
    }

    std::shared_ptr<MethodDefinition> ObjectInstance::findMethodInHierarchy(const std::string& methodName, size_t argCount) const {
        return classDefinition ? classDefinition->findMethodInHierarchy(methodName, argCount) : nullptr;
    }

} // namespace runtimeTypes::klass
