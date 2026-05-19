#include "ObjectInstance.hpp"
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include "../gc/GC.hpp"
#include "ValueBridge.hpp"
#include "ValueObject.hpp"
#include "ValueShim.hpp"

namespace runtimeTypes::klass
{
    void ObjectInstance::loadFromValueObject(const value::ValueObject& src)
    {
        if (!classDefinition) return;

        // Direct vector copy: std::vector::operator= retains heap-typed Values
        // exactly once (one atomic per heap field).
        const auto& srcFields = src.getFields();
        fieldVector = srcFields;

        // Generic-type bindings carry over too — the previous code copied
        // them via a per-entry setGenericTypeBinding loop that landed in the
        // same hashmap; assignment is one bucket-copy.
        const auto& srcBindings = src.getGenericTypeBindings();
        if (!srcBindings.empty())
        {
            genericTypeBindings = srcBindings;
        }
    }

    void ObjectInstance::visitReferences(const std::function<void(void*)>& callback) const
    {
        for (const auto& fieldValue : fieldVector)
        {
            void* ptr = gc::extractPointer(fieldValue);
            if (ptr)
            {
                callback(ptr);
            }
        }

        for (const auto& pair : fieldValues)
        {
            void* ptr = gc::extractPointer(pair.second);
            if (ptr)
            {
                callback(ptr);
            }
        }

        if (specializedCollection_)
        {
            specializedCollection_->visitReferences(callback);
        }
    }

    std::string ObjectInstance::getFullTypeName() const
    {
        if (!classDefinition) {
            return "unknown";
        }

        std::string baseName = classDefinition->getName();

        // If the class name already contains type arguments (e.g. "Pair<Int,String>")
        // it is an already-instantiated generic class — return as-is.
        if (baseName.find('<') != std::string::npos && baseName.find('>') != std::string::npos) {
            return baseName;
        }

        if (genericTypeBindings.empty()) {
            return baseName;
        }

        const auto& genericParams = classDefinition->getGenericParameters();

        if (genericParams.empty()) {
            return baseName;
        }

        std::string fullName = baseName + "<";
        bool first = true;

        for (const auto& param : genericParams) {
            if (!first) {
                fullName += ",";  // No space after comma to match bytecode format
            }
            first = false;

            auto it = genericTypeBindings.find(param.name);
            if (it != genericTypeBindings.end()) {
                fullName += it->second;
            } else {
                fullName += param.name;
            }
        }

        fullName += ">";
        return fullName;
    }

    std::string ObjectInstance::getContentHash() const
    {
        return getContentHashImpl(0);
    }

    std::string ObjectInstance::getContentHashImpl(int depth) const
    {
        static constexpr int MAX_HASH_DEPTH = 10;

        std::string hash = classDefinition->getName() + ":";

        if (depth >= MAX_HASH_DEPTH) {
            hash += "<circular>";
            return hash;
        }

        // Sort field names for stable hashing.
        const auto& fields = classDefinition->getInstanceFields();
        std::vector<std::string> fieldNames;
        for (const auto& pair : fields) {
            if (!pair.second->isStatic()) {
                fieldNames.push_back(pair.first);
            }
        }
        std::sort(fieldNames.begin(), fieldNames.end());

        for (const std::string& fieldName : fieldNames) {
            Value fieldValue = getFieldValue(fieldName);
            hash += fieldName + "=";

            // Tag-driven content hashing. Strings hash by content (both STD
            // and interned), nested objects recurse with the shared depth cap,
            // and other heap kinds fall back to pointer-based hashing.
            if (value::isInt(fieldValue)) hash += std::to_string(value::asInt(fieldValue));
            else if (value::isFloat(fieldValue)) hash += std::to_string(value::asFloat(fieldValue));
            else if (value::isBool(fieldValue)) hash += value::asBool(fieldValue) ? "true" : "false";
            else if (value::isVoid(fieldValue)) hash += "void";
            else if (value::isNullType(fieldValue)) hash += "null";
            // MYT-317: SSO-aware — append all string forms via string_view.
            else if (value::isAnyString(fieldValue)) hash.append(value::asStringView(fieldValue));
            else if (value::isObject(fieldValue))
            {
                auto obj = value::asObject(fieldValue);
                hash += obj ? obj->getContentHashImpl(depth + 1) : "null_obj";
            }
            else
            {
                hash += "ref_" + std::to_string(reinterpret_cast<uintptr_t>(fieldValue.rawBridge()));
            }
            hash += ";";
        }

        return hash;
    }

    bool ObjectInstance::compareFieldValues(const Value& thisValue, const Value& otherValue, int depth)
    {
        // Tag-driven field comparison. Primitives and strings compare by
        // value (Value::operator== already handles content-correct equality).
        // Nested objects recurse via contentEqualsImpl with the shared depth
        // cap. All other heap kinds return false for distinct Values.
        if (thisValue.tag() != otherValue.tag()) return false;
        switch (thisValue.tag())
        {
        case value::ValueType::INT:
        case value::ValueType::FLOAT:
        case value::ValueType::BOOL:
        case value::ValueType::VOID:
        case value::ValueType::NULL_TYPE:
        case value::ValueType::STRING:
            return thisValue == otherValue;
        case value::ValueType::OBJECT:
        {
            auto lhs = value::asObject(thisValue);
            auto rhs = value::asObject(otherValue);
            if (!lhs && !rhs) return true;
            if (!lhs || !rhs) return false;
            return lhs->contentEqualsImpl(*rhs, depth + 1);
        }
        default:
            return false;
        }
    }

    bool ObjectInstance::contentEquals(const ObjectInstance& other) const
    {
        return contentEqualsImpl(other, 0);
    }

    bool ObjectInstance::contentEqualsImpl(const ObjectInstance& other, int depth) const
    {
        static constexpr int MAX_EQUALS_DEPTH = 10;

        if (!classDefinition || !other.classDefinition) {
            return false;
        }
        if (classDefinition->getName() != other.classDefinition->getName()) {
            return false;
        }

        // Cap recursion on circular references; assume equal at the limit.
        if (depth >= MAX_EQUALS_DEPTH) {
            return true;
        }

        const auto& fields = classDefinition->getInstanceFields();

        for (const auto& pair : fields) {
            if (!pair.second->isStatic()) {
                const std::string& fieldName = pair.first;
                Value thisValue = getFieldValue(fieldName);
                Value otherValue = other.getFieldValue(fieldName);

                if (!compareFieldValues(thisValue, otherValue, depth)) {
                    return false;
                }
            }
        }

        return true;
    }

    void ObjectInstance::setGenericTypeBinding(const std::string& parameter, const std::string& concreteType)
    {
        genericTypeBindings[parameter] = concreteType;
    }

    std::string ObjectInstance::resolveGenericType(const std::string& typeName) const
    {
        auto it = genericTypeBindings.find(typeName);
        if (it != genericTypeBindings.end()) {
            return it->second;
        }

        // Handle generic class names like "Node<T>" -> "Node<String>".
        if (typeName.find('<') != std::string::npos) {
            std::string resolved = typeName;

            for (const auto& binding : genericTypeBindings) {
                const std::string& param = binding.first;
                const std::string& concrete = binding.second;

                size_t pos = 0;
                while ((pos = resolved.find('<' + param + '>', pos)) != std::string::npos) {
                    resolved.replace(pos + 1, param.length(), concrete);
                    pos += 1 + concrete.length();
                }

                // Also handle cases like "Node<T," -> "Node<String,".
                pos = 0;
                while ((pos = resolved.find('<' + param + ',', pos)) != std::string::npos) {
                    resolved.replace(pos + 1, param.length(), concrete);
                    pos += 1 + concrete.length();
                }
            }

            return resolved;
        }

        return typeName;
    }

    const std::unordered_map<std::string, std::string>& ObjectInstance::getGenericTypeBindings() const
    {
        return genericTypeBindings;
    }

    void ObjectInstance::attachSpecializedCollection(
        value::SpecializedCollectionStorage::Kind kind,
        value::PrimitiveTypeTag keyTag,
        size_t initialCapacity)
    {
        specializedCollection_ =
            std::make_unique<value::SpecializedCollectionStorage>(kind, keyTag, initialCapacity);
    }

    void ObjectInstance::attachSpecializedShapeCollection(
        value::SpecializedCollectionStorage::Kind kind,
        value::ShapeDescriptor shape,
        size_t initialCapacity)
    {
        if (shape.empty()) return;
        specializedCollection_ =
            std::make_unique<value::SpecializedCollectionStorage>(kind, std::move(shape), initialCapacity);
    }

    void ObjectInstance::clearSpecializedCollection()
    {
        specializedCollection_.reset();
    }
}
