#pragma once

#include "Collection.hpp"
#include <vector>
#include <stdexcept>

namespace runtimeTypes::collections
{
    class List : public Collection
    {
    private:
        std::vector<Value> elements;
        ValueType elementType;
        mutable size_t iteratorIndex = 0;

    public:
        explicit List(ValueType elemType) : elementType(elemType) {}
        
        // Collection interface implementation
        size_t size() const override { return elements.size(); }
        bool empty() const override { return elements.empty(); }
        void clear() override { elements.clear(); }
        std::string getTypeName() const override { return "List"; }
        ValueType getElementType() const override { return elementType; }
        
        // Iterator support
        bool hasNext() const override { return iteratorIndex < elements.size(); }
        Value getNext() override {
            if (iteratorIndex >= elements.size()) {
                throw std::runtime_error("List iterator out of bounds");
            }
            return elements[iteratorIndex++];
        }
        void resetIterator() override { iteratorIndex = 0; }
        
        // List-specific operations
        void add(const Value& value) {
            validateType(value);
            elements.push_back(value);
        }
        
        void insert(size_t index, const Value& value) {
            if (index > elements.size()) {
                throw std::runtime_error("List index out of bounds");
            }
            validateType(value);
            elements.insert(elements.begin() + index, value);
        }
        
        Value get(size_t index) const {
            if (index >= elements.size()) {
                throw std::runtime_error("List index out of bounds");
            }
            return elements[index];
        }
        
        void set(size_t index, const Value& value) {
            if (index >= elements.size()) {
                throw std::runtime_error("List index out of bounds");
            }
            validateType(value);
            elements[index] = value;
        }
        
        void removeAt(size_t index) {
            if (index >= elements.size()) {
                throw std::runtime_error("List index out of bounds");
            }
            elements.erase(elements.begin() + index);
        }
        
        void removeLast() {
            if (elements.empty()) {
                throw std::runtime_error("Cannot remove from empty list");
            }
            elements.pop_back();
        }

    private:
        void validateType(const Value& value) const {
            if (getValueType(value) != elementType) {
                throw std::runtime_error("Type mismatch: expected " + 
                    valueTypeToString(elementType) + " but got " + 
                    valueTypeToString(getValueType(value)));
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