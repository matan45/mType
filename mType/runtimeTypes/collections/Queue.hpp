#pragma once

#include "Collection.hpp"
#include <queue>
#include <deque>
#include <stdexcept>

namespace runtimeTypes::collections
{
    class Queue : public Collection
    {
    private:
        std::queue<Value> elements;
        ValueType elementType;
        
        // For iterator support, we need to copy elements to a vector
        mutable std::vector<Value> iteratorCopy;
        mutable size_t iteratorIndex = 0;
        mutable bool iteratorPrepared = false;

    public:
        explicit Queue(ValueType elemType) : elementType(elemType) {}
        
        // Collection interface implementation
        size_t size() const override { return elements.size(); }
        bool empty() const override { return elements.empty(); }
        void clear() override { 
            std::queue<Value> empty;
            elements.swap(empty);
        }
        std::string getTypeName() const override { return "Queue"; }
        ValueType getElementType() const override { return elementType; }
        
        // Iterator support
        bool hasNext() const override {
            prepareIterator();
            return iteratorIndex < iteratorCopy.size();
        }
        
        Value getNext() override {
            if (!hasNext()) {
                throw std::runtime_error("Queue iterator out of bounds");
            }
            return iteratorCopy[iteratorIndex++];
        }
        
        void resetIterator() override {
            iteratorIndex = 0;
            iteratorPrepared = false;
        }
        
        // Queue-specific operations
        void enqueue(const Value& value) {
            validateType(value);
            elements.push(value);
        }
        
        Value dequeue() {
            if (elements.empty()) {
                throw std::runtime_error("Cannot dequeue from empty queue");
            }
            Value front = elements.front();
            elements.pop();
            return front;
        }
        
        Value front() const {
            if (elements.empty()) {
                throw std::runtime_error("Cannot access front of empty queue");
            }
            return elements.front();
        }

    private:
        void prepareIterator() const {
            if (!iteratorPrepared) {
                iteratorCopy.clear();
                std::queue<Value> temp = elements;  // Copy the queue
                while (!temp.empty()) {
                    iteratorCopy.push_back(temp.front());
                    temp.pop();
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