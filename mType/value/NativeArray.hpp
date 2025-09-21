#pragma once
#include "ValueType.hpp"
#include <vector>
#include <memory>

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

    public:
        explicit NativeArray(size_t size) : data(size) {}

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
    };
}