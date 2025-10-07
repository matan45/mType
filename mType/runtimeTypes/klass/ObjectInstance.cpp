#include "ObjectInstance.hpp"
#include <algorithm>
#include <vector>

namespace runtimeTypes::klass
{
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
                field->setValue(value);
            } else {
                // For instance fields, set value in this instance's storage
                fieldValues[fieldName] = value;
            }
        } else {
            // Allow setting dynamic fields that aren't part of the class definition
            // This is needed for lambda backing fields
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
                
                // Deep comparison of field values
                bool fieldsEqual = std::visit([](const auto& thisV, const auto& otherV) -> bool {
                    // Same types comparison
                    if constexpr (std::is_same_v<std::decay_t<decltype(thisV)>, std::decay_t<decltype(otherV)>>) {
                        if constexpr (std::is_same_v<std::decay_t<decltype(thisV)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>) {
                            // Recursive content comparison for nested objects
                            if (!thisV && !otherV) return true;
                            if (!thisV || !otherV) return false;
                            return thisV->contentEquals(*otherV);
                        } else if constexpr (std::is_same_v<std::decay_t<decltype(thisV)>, int> ||
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
                
                if (!fieldsEqual) {
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