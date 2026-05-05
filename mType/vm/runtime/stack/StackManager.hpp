#pragma once
#include <vector>
#include <cstddef>
#include "../../../value/ValueType.hpp"

namespace vm::runtime
{
    /**
     * Manages the operand stack for the virtual machine
     * Provides stack operations with underflow validation
     */
    class StackManager
    {
    public:
        StackManager();
        ~StackManager() = default;

        // Stack operations
        void push(const value::Value& value);
        void push(value::Value&& value);
        value::Value pop();
        value::Value peek(size_t offset = 0) const;
        void popN(size_t count);

        // Stack state
        size_t size() const;
        bool empty() const;
        void clear();
        void reserve(size_t capacity);

        // Direct access (for frame-based operations)
        value::Value& operator[](size_t index);
        const value::Value& operator[](size_t index) const;
        std::vector<value::Value>& getStack();
        const std::vector<value::Value>& getStack() const;
        void setStack(const std::vector<value::Value>& newStack);

        // Stack size manipulation (for call frames)
        void resize(size_t newSize);
        void resize(size_t newSize, const value::Value& defaultValue);

    private:
        std::vector<value::Value> stack;

        void checkStackUnderflow(size_t required) const;
    };
}
