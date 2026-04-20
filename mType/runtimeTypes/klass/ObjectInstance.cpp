#include "ObjectInstance.hpp"
#include "../../gc/GC.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/ValueBridge.hpp"
#include <algorithm>
#include <cstddef>
#include <vector>

namespace runtimeTypes::klass
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4413)  // offsetof on non-standard-layout (enable_shared_from_this base)
#endif
    size_t ObjectInstance::classDefinitionMemberOffset() noexcept
    {
        return offsetof(ObjectInstance, classDefinition);
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    void ObjectInstance::registerWithGC()
    {
        if (!gcRegistered && gc::GC::isInitialized())
        {
            gc::GC::registerAllocation(shared_from_this(), gc::config::GCObjectType::OBJECT_INSTANCE);
            gcRegistered = true;
        }
    }

    void ObjectInstance::resetForRecycle()
    {
        // unordered_map::clear() destroys nodes (releasing Value bridges and
        // shared_ptr<MethodDefinition> entries) but keeps the bucket array.
        fieldValues.clear();
        methodCache.clear();
        genericTypeBindings.clear();
        fieldVector.clear();
        fieldVectorInitialized = false;
        gcRegistered = false;
        primitiveTag_ = value::PrimitiveTypeTag::NONE;
        classDefinition.reset();
    }

    void ObjectInstance::reinitForRecycle(
        std::shared_ptr<ClassDefinition> classDef,
        const std::unordered_map<std::string, std::string>& typeBindings)
    {
        classDefinition = std::move(classDef);
        if (!typeBindings.empty())
        {
            genericTypeBindings = typeBindings;
        }
        if (classDefinition)
        {
            fieldValues.reserve(classDefinition->getTotalFieldCount());
            primitiveTag_ = value::classNameToPrimitiveTag(classDefinition->getName());
            if (primitiveTag_ != value::PrimitiveTypeTag::NONE)
            {
                ensureFieldVector();
            }
        }
    }
    std::shared_ptr<FieldDefinition> ObjectInstance::getField(const std::string& fieldName) const
    {
        // Search in class hierarchy to support inherited fields
        return classDefinition->getFieldInHierarchy(fieldName);
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
        } else {
            // Check for dynamic fields like lambda backing fields
            auto it = fieldValues.find(fieldName);
            if (it != fieldValues.end()) {
                return it->second;
            }
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
                // GC: Write barrier for static field
                Value oldValue = field->getValue();
                void* oldPtr = gc::extractPointer(oldValue);
                void* newPtr = gc::extractPointer(value);
                if (oldPtr != nullptr && oldPtr != newPtr)
                {
                    gc::GC::onRefCountDecrement(oldPtr);
                }
                field->setValue(value);
            } else {
                // For instance fields, set value in this instance's storage
                // GC: Write barrier for instance field
                auto it = fieldValues.find(fieldName);
                void* newPtr = gc::extractPointer(value);
                if (it != fieldValues.end())
                {
                    void* oldPtr = gc::extractPointer(it->second);
                    if (oldPtr != nullptr && oldPtr != newPtr)
                    {
                        gc::GC::onRefCountDecrement(oldPtr);
                    }
                }
                // If storing an object reference, this object becomes a potential cycle root
                if (newPtr != nullptr && gcRegistered)
                {
                    gc::GC::onRefCountDecrement(this);
                }
                fieldValues[fieldName] = value;

                // Phase 6 (IC): Keep fieldVector in sync
                if (fieldVectorInitialized)
                {
                    size_t idx = classDefinition->getFieldIndex(fieldName);
                    if (idx != SIZE_MAX && idx < fieldVector.size())
                    {
                        fieldVector[idx] = value;
                    }
                }
            }
        } else {
            // Allow setting dynamic fields that aren't part of the class definition
            // This is needed for lambda backing fields
            // GC: Write barrier for dynamic field
            auto it = fieldValues.find(fieldName);
            void* newPtr = gc::extractPointer(value);
            if (it != fieldValues.end())
            {
                void* oldPtr = gc::extractPointer(it->second);
                if (oldPtr != nullptr && oldPtr != newPtr)
                {
                    gc::GC::onRefCountDecrement(oldPtr);
                }
            }
            // If storing an object reference, this object becomes a potential cycle root
            if (newPtr != nullptr && gcRegistered)
            {
                gc::GC::onRefCountDecrement(this);
            }
            fieldValues[fieldName] = value;
        }
    }

    void ObjectInstance::ensureFieldVector() const
    {
        if (fieldVectorInitialized) return;
        if (!classDefinition) return;

        const auto& indexMap = classDefinition->getFieldIndexMap();
        size_t count = classDefinition->getIndexedFieldCount();
        fieldVector.resize(count, std::monostate{});

        // Populate from existing fieldValues
        for (const auto& [name, value] : fieldValues)
        {
            auto it = indexMap.find(name);
            if (it != indexMap.end() && it->second < count)
            {
                fieldVector[it->second] = value;
            }
        }

        fieldVectorInitialized = true;
    }

    const Value& ObjectInstance::getFieldByIndex(size_t index) const
    {
        static const Value nullValue = std::monostate{};
        if (index >= fieldVector.size()) return nullValue;
        return fieldVector[index];
    }

    void ObjectInstance::setFieldByIndex(size_t index, const Value& value)
    {
        if (index >= fieldVector.size()) return;

        // GC: Write barrier
        void* newPtr = gc::extractPointer(value);
        void* oldPtr = gc::extractPointer(fieldVector[index]);
        if (oldPtr != nullptr && oldPtr != newPtr)
        {
            gc::GC::onRefCountDecrement(oldPtr);
        }
        if (newPtr != nullptr && gcRegistered)
        {
            gc::GC::onRefCountDecrement(this);
        }

        fieldVector[index] = value;

        // Keep fieldValues map in sync
        if (!classDefinition) return;
        const auto& indexMap = classDefinition->getFieldIndexMap();
        for (const auto& [name, idx] : indexMap)
        {
            if (idx == index)
            {
                fieldValues[name] = value;
                break;
            }
        }
    }

    bool ObjectInstance::isInstanceOf(const std::string& className) const
    {
        if (!classDefinition) {
            return false;
        }

        // Check direct class match
        if (classDefinition->getName() == className) {
            return true;
        }

        // NEW: Check inheritance chain
        return classDefinition->isSubclassOf(className);
    }

    std::string ObjectInstance::getTypeName() const
    {
        return classDefinition ? classDefinition->getName() : "unknown";
    }

    std::string ObjectInstance::getFullTypeName() const
    {
        if (!classDefinition) {
            return "unknown";
        }

        std::string baseName = classDefinition->getName();

        // Check if the class name already contains type arguments (e.g., "Pair<Int,String>")
        // This happens when the class is an instantiated generic class
        if (baseName.find('<') != std::string::npos && baseName.find('>') != std::string::npos) {
            // Already a full generic type name, return as-is
            return baseName;
        }

        // If no generic type bindings, return just the base name
        if (genericTypeBindings.empty()) {
            return baseName;
        }

        // Construct full generic type name (e.g., "Box<String>")
        // We need to get the generic parameters in the correct order
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

            // Look up the concrete type for this parameter
            auto it = genericTypeBindings.find(param.name);
            if (it != genericTypeBindings.end()) {
                fullName += it->second;
            } else {
                fullName += param.name;  // Fallback to parameter name if not bound
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

            // Tag-driven content hashing. Strings hash by content (both STD
            // and interned), nested objects recurse with the shared depth cap,
            // and other heap kinds fall back to pointer-based hashing.
            if (value::isInt(fieldValue)) hash += std::to_string(value::asInt(fieldValue));
            else if (value::isFloat(fieldValue)) hash += std::to_string(value::asFloat(fieldValue));
            else if (value::isBool(fieldValue)) hash += value::asBool(fieldValue) ? "true" : "false";
            else if (value::isVoid(fieldValue)) hash += "void";
            else if (value::isNullType(fieldValue)) hash += "null";
            else if (value::isString(fieldValue)) hash += value::asString(fieldValue);
            else if (value::isInternedString(fieldValue)) hash += value::asInternedString(fieldValue).getString();
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

        // First check class compatibility
        if (!classDefinition || !other.classDefinition) {
            return false;
        }
        if (classDefinition->getName() != other.classDefinition->getName()) {
            return false;
        }

        // Prevent infinite recursion on circular references
        if (depth >= MAX_EQUALS_DEPTH) {
            return true;  // Assume equal at max depth
        }

        // Get all instance fields for comparison
        const auto& fields = classDefinition->getInstanceFields();

        // Compare all instance field values
        for (const auto& pair : fields) {
            if (!pair.second->isStatic()) {  // Only instance fields
                const std::string& fieldName = pair.first;
                Value thisValue = getFieldValue(fieldName);
                Value otherValue = other.getFieldValue(fieldName);

                if (!compareFieldValues(thisValue, otherValue, depth)) {
                    return false;
                }
            }
        }

        return true;  // All fields match
    }

    void ObjectInstance::setGenericTypeBinding(const std::string& parameter, const std::string& concreteType)
    {
        genericTypeBindings[parameter] = concreteType;
    }

    std::string ObjectInstance::resolveGenericType(const std::string& typeName) const
    {
        // Check if this is a generic type parameter that needs to be resolved
        auto it = genericTypeBindings.find(typeName);
        if (it != genericTypeBindings.end()) {
            return it->second;
        }

        // Handle generic class names like "Node<T>" -> "Node<String>"
        if (typeName.find('<') != std::string::npos) {
            std::string resolved = typeName;

            // Find and replace generic type parameters with their concrete bindings
            for (const auto& binding : genericTypeBindings) {
                const std::string& param = binding.first;
                const std::string& concrete = binding.second;

                // Replace <T> with <String>, etc.
                size_t pos = 0;
                while ((pos = resolved.find('<' + param + '>', pos)) != std::string::npos) {
                    resolved.replace(pos + 1, param.length(), concrete);
                    pos += 1 + concrete.length();
                }

                // Also handle cases like "Node<T," -> "Node<String,"
                pos = 0;
                while ((pos = resolved.find('<' + param + ',', pos)) != std::string::npos) {
                    resolved.replace(pos + 1, param.length(), concrete);
                    pos += 1 + concrete.length();
                }
            }

            return resolved;
        }

        // If not a generic type, return as-is
        return typeName;
    }

    const std::unordered_map<std::string, std::string>& ObjectInstance::getGenericTypeBindings() const
    {
        return genericTypeBindings;
    }

    std::shared_ptr<MethodDefinition> ObjectInstance::findMethodInHierarchy(const std::string& methodName, size_t argCount) const
    {
        if (!classDefinition) {
            return nullptr;
        }

        // Create cache key
        std::string cacheKey = methodName + "_" + std::to_string(argCount);

        // Check cache first
        auto cacheIt = methodCache.find(cacheKey);
        if (cacheIt != methodCache.end()) {
            return cacheIt->second;
        }

        // Search using ClassDefinition's hierarchy search
        auto method = classDefinition->findMethodInHierarchy(methodName, argCount);

        // Cache the result (even if nullptr to avoid repeated searches)
        methodCache[cacheKey] = method;

        return method;
    }
}