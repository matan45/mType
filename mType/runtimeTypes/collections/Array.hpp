#pragma once

#include "Collection.hpp"
#include "../klass/ObjectInstance.hpp"
#include <vector>
#include <stdexcept>

namespace runtimeTypes::collections
{
    class Array : public Collection
    {
    private:
        std::vector<Value> elements;
        ValueType elementType;
        std::string expectedClassName;  // Store expected class name for object types
        mutable size_t iteratorIndex = 0;

    public:
        explicit Array(ValueType elemType) : elementType(elemType), expectedClassName("") {}
        
        // Constructor for specific class types
        Array(ValueType elemType, const std::string& className) 
            : elementType(elemType), expectedClassName(className) {}
        
        // Collection interface implementation
        size_t size() const override { return elements.size(); }
        bool empty() const override { return elements.empty(); }
        void clear() override { elements.clear(); }
        std::string getTypeName() const override { return "Array"; }
        ValueType getElementType() const override { return elementType; }
        
        // Iterator support
        bool hasNext() const override { return iteratorIndex < elements.size(); }
        Value getNext() override {
            if (iteratorIndex >= elements.size()) {
                throw std::runtime_error("Array iterator out of bounds");
            }
            return elements[iteratorIndex++];
        }
        void resetIterator() override { iteratorIndex = 0; }
        
        // Array-specific operations
        void add(const Value& value) {
            validateType(value);
            elements.push_back(value);
        }
        
        Value get(size_t index) const {
            if (index >= elements.size()) {
                throw std::runtime_error("Array index out of bounds");
            }
            return elements[index];
        }
        
        void set(size_t index, const Value& value) {
            if (index >= elements.size()) {
                throw std::runtime_error("Array index out of bounds");
            }
            validateType(value);
            elements[index] = value;
        }
        
        void removeAt(size_t index) {
            if (index >= elements.size()) {
                throw std::runtime_error("Array index out of bounds");
            }
            elements.erase(elements.begin() + index);
        }
        
        // Index operator for bracket access
        Value operator[](size_t index) const { return get(index); }

    private:
        void validateType(const Value& value) const {
            ValueType actualType = value::getValueType(value);
            
            // Special handling for collections as objects (for nested collections)
            if (elementType == ValueType::OBJECT && !expectedClassName.empty()) {
                
                // Check if we're expecting a collection type and got a collection
                if (expectedClassName.find("Array<") == 0 || 
                    expectedClassName.find("Map<") == 0 || expectedClassName.find("Set<") == 0 ||
                    expectedClassName.find("Queue<") == 0 || expectedClassName.find("Stack<") == 0) {
                    
                    // For nested collections, validate the collection type matches
                    bool validCollection = false;
                    
                    if (expectedClassName.find("Array<") == 0 && 
                        std::holds_alternative<std::shared_ptr<runtimeTypes::collections::Array>>(value)) {
                        validCollection = true;
                    }
                    else if (expectedClassName.find("Map<") == 0 && 
                             std::holds_alternative<std::shared_ptr<runtimeTypes::collections::Map>>(value)) {
                        validCollection = true;
                    }
                    // Add other collection types as needed
                    
                    if (!validCollection) {
                        throw std::runtime_error("Collection type mismatch: expected " + 
                            expectedClassName + " but got " + valueTypeToString(actualType));
                    }
                    return; // Valid collection, skip further validation
                }
                
                // Handle regular object validation
                if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value)) {
                    auto objectInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value);
                    if (objectInstance && !objectInstance->isInstanceOf(expectedClassName)) {
                        throw std::runtime_error("Object type mismatch: expected instance of '" + 
                            expectedClassName + "' but got instance of '" + 
                            objectInstance->getTypeName() + "'");
                    }
                }
                return;
            }
            
            // Basic type check for non-nested collections
            if (actualType != elementType) {
                throw std::runtime_error("Type mismatch: expected " + 
                    valueTypeToString(elementType) + " but got " + 
                    valueTypeToString(actualType));
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