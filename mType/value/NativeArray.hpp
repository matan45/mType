#pragma once
#include "ValueType.hpp"
#include <vector>
#include <memory>
#include <string>

namespace value
{
    /**
     * @brief Native array implementation for mType language
     * Supports indexing access and assignment for arrays created with new T[size]
     */
    class NativeArray
    {
    private:
        std::vector<Value> data;
        ValueType elementType;
        std::string elementTypeName;  // For object types, stores class/interface name

    private:
        static Value getDefaultValueForType(ValueType type) {
            switch (type) {
                case ValueType::INT: return 0;
                case ValueType::FLOAT: return 0.0f;
                case ValueType::BOOL: return false;
                case ValueType::STRING: return std::string("");
                case ValueType::OBJECT: return std::monostate{}; // null for objects
                case ValueType::VOID: return std::monostate{};
                default: return std::monostate{};
            }
        }

    public:
        explicit NativeArray(size_t size) : data(size), elementType(ValueType::VOID), elementTypeName("") {
            // Initialize all elements to null
            for (auto& elem : data) {
                elem = std::monostate{};
            }
        }

        explicit NativeArray(size_t size, ValueType elemType, const std::string& elemTypeName = "")
            : data(size), elementType(elemType), elementTypeName(elemTypeName) {
            // Initialize all elements based on type
            Value defaultValue = getDefaultValueForType(elemType);
            for (auto& elem : data) {
                elem = defaultValue;
            }
        }

        // Index access
        Value& operator[](size_t index) { return data[index]; }
        const Value& operator[](size_t index) const { return data[index]; }

        // Safe access with bounds checking
        Value get(size_t index) const {
            if (index >= data.size()) {
                return std::monostate{}; // Return null for out of bounds
            }
            return data[index];
        }

        void set(size_t index, const Value& value) {
            if (index < data.size()) {
                data[index] = value;
            }
        }

        size_t size() const { return data.size(); }

        // Element type information
        ValueType getElementType() const { return elementType; }
        const std::string& getElementTypeName() const { return elementTypeName; }
        void setElementType(ValueType elemType, const std::string& elemTypeName = "") {
            elementType = elemType;
            elementTypeName = elemTypeName;
        }
    };
}