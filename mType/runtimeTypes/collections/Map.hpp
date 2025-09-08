#pragma once

#include "Collection.hpp"
#include <unordered_map>
#include <stdexcept>

namespace runtimeTypes::collections
{
    class Map : public Collection
    {
    private:
        std::unordered_map<std::string, Value> elements;  // For simplicity, using string keys only
        ValueType keyType;
        ValueType valueType;
        mutable std::unordered_map<std::string, Value>::const_iterator iterator;
        mutable bool iteratorValid = false;

    public:
        Map(ValueType keyType, ValueType valType) : keyType(keyType), valueType(valType) {}
        
        // Collection interface implementation
        size_t size() const override { return elements.size(); }
        bool empty() const override { return elements.empty(); }
        void clear() override { elements.clear(); }
        std::string getTypeName() const override { return "Map"; }
        ValueType getElementType() const override { return valueType; }
        
        // Iterator support (iterates over values)
        bool hasNext() const override {
            if (!iteratorValid) {
                iterator = elements.begin();
                iteratorValid = true;
            }
            return iterator != elements.end();
        }
        
        Value getNext() override {
            if (!hasNext()) {
                throw std::runtime_error("Map iterator out of bounds");
            }
            Value result = iterator->second;
            ++iterator;
            return result;
        }
        
        void resetIterator() override {
            iterator = elements.begin();
            iteratorValid = true;
        }
        
        // Map-specific operations
        void put(const std::string& key, const Value& value) {
            validateValueType(value);
            elements[key] = value;
        }
        
        Value get(const std::string& key) const {
            auto it = elements.find(key);
            if (it == elements.end()) {
                throw std::runtime_error("Map key not found: " + key);
            }
            return it->second;
        }
        
        bool containsKey(const std::string& key) const {
            return elements.find(key) != elements.end();
        }
        
        void remove(const std::string& key) {
            elements.erase(key);
        }
        
        ValueType getKeyType() const { return keyType; }
        ValueType getValueType() const { return valueType; }

    private:
        void validateValueType(const Value& value) const {
            if (value::getValueType(value) != valueType) {
                throw std::runtime_error("Type mismatch: expected " + 
                    valueTypeToString(valueType) + " but got " + 
                    valueTypeToString(value::getValueType(value)));
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