#pragma once

#include "Collection.hpp"
#include <stack>
#include <vector>
#include <stdexcept>

namespace runtimeTypes::collections
{
    class Stack : public Collection
    {
    private:
        std::stack<Value> elements;
        ValueType elementType;
        
        // For iterator support, we need to copy elements to a vector
        mutable std::vector<Value> iteratorCopy;
        mutable size_t iteratorIndex = 0;
        mutable bool iteratorPrepared = false;

    public:
        explicit Stack(ValueType elemType) : elementType(elemType) {}
        
        // Collection interface implementation
        size_t size() const override { return elements.size(); }
        bool empty() const override { return elements.empty(); }
        void clear() override { 
            std::stack<Value> empty;
            elements.swap(empty);
        }
        std::string getTypeName() const override { return "Stack"; }
        ValueType getElementType() const override { return elementType; }
        
        // Iterator support (iterates from bottom to top)
        bool hasNext() const override {
            prepareIterator();
            return iteratorIndex < iteratorCopy.size();
        }
        
        Value getNext() override {
            if (!hasNext()) {
                throw std::runtime_error("Stack iterator out of bounds");
            }
            return iteratorCopy[iteratorIndex++];
        }
        
        void resetIterator() override {
            iteratorIndex = 0;
            iteratorPrepared = false;
        }
        
        // Stack-specific operations
        void push(const Value& value) {
            validateType(value);
            elements.push(value);
        }
        
        Value pop() {
            if (elements.empty()) {
                throw std::runtime_error("Cannot pop from empty stack");
            }
            Value top = elements.top();
            elements.pop();
            return top;
        }
        
        Value top() const {
            if (elements.empty()) {
                throw std::runtime_error("Cannot access top of empty stack");
            }
            return elements.top();
        }

    private:
        void prepareIterator() const {
            if (!iteratorPrepared) {
                iteratorCopy.clear();
                std::stack<Value> temp = elements;  // Copy the stack
                std::vector<Value> tempVec;
                
                // Extract all elements (this reverses the order)
                while (!temp.empty()) {
                    tempVec.push_back(temp.top());
                    temp.pop();
                }
                
                // Reverse to get bottom-to-top order for iteration
                for (int i = tempVec.size() - 1; i >= 0; --i) {
                    iteratorCopy.push_back(tempVec[i]);
                }
                
                iteratorPrepared = true;
            }
        }
        
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