#include "ObjectInstance.hpp"
#include "../../gc/GC.hpp"
#include <algorithm>
#include <vector>

namespace runtimeTypes::klass
{
    void ObjectInstance::registerWithGC()
    {
        if (!gcRegistered && gc::GC::isInitialized())
        {
            gc::GC::registerAllocation(shared_from_this(), gc::config::GCObjectType::OBJECT_INSTANCE);
            gcRegistered = true;
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
                if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int64_t>) {
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

    bool ObjectInstance::compareFieldValues(const Value& thisValue, const Value& otherValue)
    {
        return std::visit([](const auto& thisV, const auto& otherV) -> bool {
            // Same types comparison
            if constexpr (std::is_same_v<std::decay_t<decltype(thisV)>, std::decay_t<decltype(otherV)>>) {
                if constexpr (std::is_same_v<std::decay_t<decltype(thisV)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>) {
                    // Recursive content comparison for nested objects
                    if (!thisV && !otherV) return true;
                    if (!thisV || !otherV) return false;
                    return thisV->contentEquals(*otherV);
                } else if constexpr (std::is_same_v<std::decay_t<decltype(thisV)>, int64_t> ||
                                   std::is_same_v<std::decay_t<decltype(thisV)>, float> ||
                                   std::is_same_v<std::decay_t<decltype(thisV)>, bool> ||
                                   std::is_same_v<std::decay_t<decltype(thisV)>, std::string>) {
                    return thisV == otherV;
                } else if constexpr (std::is_same_v<std::decay_t<decltype(thisV)>, nullptr_t> ||
                                   std::is_same_v<std::decay_t<decltype(thisV)>, std::monostate>) {
                    return true;  // Both null or void
                } else {
                    // For collections and other complex types, use pointer comparison
                    // This is acceptable since full deep comparison would be expensive
                    return &thisV == &otherV;
                }
            }
            // Different types are not equal
            return false;
        }, thisValue, otherValue);
    }

    bool ObjectInstance::contentEquals(const ObjectInstance& other) const
    {
        // First check class compatibility
        if (!classDefinition || !other.classDefinition) {
            return false;
        }
        if (classDefinition->getName() != other.classDefinition->getName()) {
            return false;
        }
        
        // Get all instance fields for comparison
        const auto& fields = classDefinition->getInstanceFields();
        
        // Compare all instance field values
        for (const auto& pair : fields) {
            if (!pair.second->isStatic()) {  // Only instance fields
                const std::string& fieldName = pair.first;
                Value thisValue = getFieldValue(fieldName);
                Value otherValue = other.getFieldValue(fieldName);

                if (!compareFieldValues(thisValue, otherValue)) {
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