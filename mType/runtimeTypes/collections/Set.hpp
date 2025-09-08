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
            
            // Check if already exists
            if (stringElements.find(identifier) != stringElements.end()) {
                return false;  // Already exists
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
            std::string strValue = convertToString(value);
            return stringElements.find(strValue) != stringElements.end();
        }
        
        bool remove(const Value& value) {
            std::string identifier = convertToString(value);
            bool removed = stringElements.erase(identifier) > 0;
            
            // Also remove from object storage if it exists
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