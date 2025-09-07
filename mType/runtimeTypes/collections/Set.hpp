#pragma once

#include "Collection.hpp"
#include <unordered_set>
#include <stdexcept>

namespace runtimeTypes::collections
{
    class Set : public Collection
    {
    private:
        std::unordered_set<std::string> stringElements;  // For simplicity, string-based hash
        ValueType elementType;
        mutable std::unordered_set<std::string>::iterator iterator;
        mutable bool iteratorValid = false;

    public:
        explicit Set(ValueType elemType) : elementType(elemType) {}
        
        // Collection interface implementation
        size_t size() const override { return stringElements.size(); }
        bool empty() const override { return stringElements.empty(); }
        void clear() override { stringElements.clear(); }
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
            std::string strValue = *iterator;
            ++iterator;
            return convertFromString(strValue);
        }
        
        void resetIterator() override {
            iterator = stringElements.begin();
            iteratorValid = true;
        }
        
        // Set-specific operations
        bool add(const Value& value) {
            validateType(value);
            std::string strValue = convertToString(value);
            return stringElements.insert(strValue).second;
        }
        
        bool contains(const Value& value) const {
            std::string strValue = convertToString(value);
            return stringElements.find(strValue) != stringElements.end();
        }
        
        bool remove(const Value& value) {
            std::string strValue = convertToString(value);
            return stringElements.erase(strValue) > 0;
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
                } else {
                    return "";
                }
            }, value);
        }
        
        Value convertFromString(const std::string& str) const {
            switch (elementType) {
                case ValueType::INT: return std::stoi(str);
                case ValueType::FLOAT: return std::stof(str);
                case ValueType::BOOL: return str == "true";
                case ValueType::STRING: return str;
                default: return str;
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