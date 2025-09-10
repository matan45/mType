#pragma once

#include "Collection.hpp"
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>

namespace runtimeTypes::collections
{
    class Set : public Collection
    {
    private:
        std::unordered_set<std::string> stringElements;  // String identifiers for uniqueness checking
        std::unordered_map<std::string, Value> objectStorage;  // Actual values stored by identifier
        ValueType elementType;
        mutable std::unordered_set<std::string>::iterator iterator;
        mutable bool iteratorValid = false;

    public:
        explicit Set(ValueType elemType) : elementType(elemType) {}
        
        // Collection interface implementation
        size_t size() const override { return stringElements.size(); }
        bool empty() const override { return stringElements.empty(); }
        void clear() override { 
            stringElements.clear(); 
            objectStorage.clear();
        }
        std::string getTypeName() const override { return "Set"; }
        ValueType getElementType() const override { return elementType; }
        
        // Iterator support
        bool hasNext() const override {
            if (!iteratorValid) {
                iterator = stringElements.begin();
                iteratorValid = true;
            }
            return iterator != stringElements.end();
        }
        
        Value getNext() override {
            if (!hasNext()) {
                throw std::runtime_error("Set iterator out of bounds");
            }
            std::string identifier = *iterator;
            ++iterator;
            
            // Return actual stored value for objects, or convert from string for primitives
            if (elementType == ValueType::OBJECT || elementType == ValueType::ARRAY || 
                elementType == ValueType::MAP || elementType == ValueType::SET ||
                elementType == ValueType::QUEUE || elementType == ValueType::STACK ||
                elementType == ValueType::NULL_TYPE || elementType == ValueType::VOID) {
                auto it = objectStorage.find(identifier);
                if (it != objectStorage.end()) {
                    return it->second;
                }
                throw std::runtime_error("Set object storage corruption");
            }
            
            return convertFromString(identifier);
        }
        
        void resetIterator() override {
            iterator = stringElements.begin();
            iteratorValid = true;
        }
        
        // Set-specific operations
        bool add(const Value& value) {
            validateType(value);
            std::string identifier = convertToString(value);
            
            // Phase 1: Check hash-based uniqueness
            if (stringElements.find(identifier) != stringElements.end()) {
                // Phase 2: For objects with hash collision, perform explicit equality check
                if (elementType == ValueType::OBJECT) {
                    auto storedValue = objectStorage.find(identifier);
                    if (storedValue != objectStorage.end()) {
                        // Extract object pointers for comparison
                        auto newObj = std::get_if<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(&value);
                        auto storedObj = std::get_if<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(&storedValue->second);
                        
                        if (newObj && storedObj && *newObj && *storedObj) {
                            // Explicit content equality check
                            if ((*newObj)->contentEquals(**storedObj)) {
                                return false;  // Truly equal - don't add
                            }
                            // Hash collision but different content - need to handle this case
                            // For now, we'll consider hash collisions as duplicates to maintain uniqueness
                            // In a more sophisticated implementation, we'd use chaining or probing
                            return false;
                        }
                    }
                }
                return false;  // Already exists for primitive types
            }
            
            // Add to string set for uniqueness tracking
            bool inserted = stringElements.insert(identifier).second;
            
            // Store actual object for complex types
            if (inserted && (elementType == ValueType::OBJECT || elementType == ValueType::ARRAY || 
                           elementType == ValueType::MAP || elementType == ValueType::SET ||
                           elementType == ValueType::QUEUE || elementType == ValueType::STACK ||
                           elementType == ValueType::NULL_TYPE || elementType == ValueType::VOID)) {
                objectStorage[identifier] = value;
            }
            
            return inserted;
        }
        
        bool contains(const Value& value) const {
            std::string identifier = convertToString(value);
            
            // Phase 1: Check hash-based existence
            if (stringElements.find(identifier) == stringElements.end()) {
                return false;  // Not found by hash
            }
            
            // Phase 2: For objects, perform explicit equality check
            if (elementType == ValueType::OBJECT) {
                auto storedValue = objectStorage.find(identifier);
                if (storedValue != objectStorage.end()) {
                    // Extract object pointers for comparison
                    auto queryObj = std::get_if<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(&value);
                    auto storedObj = std::get_if<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(&storedValue->second);
                    
                    if (queryObj && storedObj && *queryObj && *storedObj) {
                        // Explicit content equality check
                        return (*queryObj)->contentEquals(**storedObj);
                    }
                }
            }
            
            return true;  // Found for primitive types or when explicit comparison not applicable
        }
        
        bool remove(const Value& value) {
            std::string identifier = convertToString(value);
            
            // Phase 1: Check if hash exists
            if (stringElements.find(identifier) == stringElements.end()) {
                return false;  // Not found by hash
            }
            
            // Phase 2: For objects, verify equality before removal
            if (elementType == ValueType::OBJECT) {
                auto storedValue = objectStorage.find(identifier);
                if (storedValue != objectStorage.end()) {
                    // Extract object pointers for comparison
                    auto queryObj = std::get_if<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(&value);
                    auto storedObj = std::get_if<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(&storedValue->second);
                    
                    if (queryObj && storedObj && *queryObj && *storedObj) {
                        // Only remove if content actually matches
                        if (!(*queryObj)->contentEquals(**storedObj)) {
                            return false;  // Hash collision but different content
                        }
                    }
                }
            }
            
            // Remove from both storage systems
            bool removed = stringElements.erase(identifier) > 0;
            if (removed) {
                objectStorage.erase(identifier);
            }
            
            return removed;
        }

    private:
        void validateType(const Value& value) const {
            if (getValueType(value) != elementType) {
                throw std::runtime_error("Type mismatch: expected " + 
                    valueTypeToString(elementType) + " but got " + 
                    valueTypeToString(getValueType(value)));
            }
        }
        
        std::string convertToString(const Value& value) const {
            return std::visit([](const auto& v) -> std::string {
                if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>) {
                    return std::to_string(v);
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, float>) {
                    return std::to_string(v);
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
                    return v ? "true" : "false";
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                    return v;
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>) {
                    // Use object content-based hash for uniqueness
                    return v ? v->getContentHash() : "null_obj";
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::collections::Array>>) {
                    return "array_" + std::to_string(reinterpret_cast<uintptr_t>(v.get()));
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::collections::Map>>) {
                    return "map_" + std::to_string(reinterpret_cast<uintptr_t>(v.get()));
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::collections::Set>>) {
                    return "set_" + std::to_string(reinterpret_cast<uintptr_t>(v.get()));
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::collections::Queue>>) {
                    return "queue_" + std::to_string(reinterpret_cast<uintptr_t>(v.get()));
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::collections::Stack>>) {
                    return "stack_" + std::to_string(reinterpret_cast<uintptr_t>(v.get()));
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, nullptr_t>) {
                    return "null";
                } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
                    return "void";
                } else {
                    return "unknown";
                }
            }, value);
        }
        
        Value convertFromString(const std::string& str) const {
            // Only used for primitive types (objects are stored directly in objectStorage)
            switch (elementType) {
                case ValueType::INT: return std::stoi(str);
                case ValueType::FLOAT: return std::stof(str);
                case ValueType::BOOL: return str == "true";
                case ValueType::STRING: return str;
                default: 
                    throw std::runtime_error("convertFromString called for non-primitive type");
            }
        }
        
        std::string valueTypeToString(ValueType type) const {
            switch (type) {
                case ValueType::INT: return "int";
                case ValueType::FLOAT: return "float";
                case ValueType::BOOL: return "bool";
                case ValueType::STRING: return "string";
                case ValueType::VOID: return "void";
                case ValueType::OBJECT: return "object";
                case ValueType::NULL_TYPE: return "null";
                default: return "unknown";
            }
        }
    };
}