#include "StackManager.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    StackManager::StackManager() {
        stack.reserve(256);  // Reserve initial capacity
    }

    void StackManager::push(const value::Value& value) {
        stack.push_back(value);
    }

    void StackManager::push(value::Value&& value) {
        stack.push_back(std::move(value));
    }

    value::Value StackManager::pop() {
        checkStackUnderflow(1);
        value::Value val = std::move(stack.back());
        stack.pop_back();
        return val;
    }

    const value::Value& StackManager::peek(size_t offset) const {
        checkStackUnderflow(offset + 1);
        return stack[stack.size() - 1 - offset];
    }

    void StackManager::popN(size_t count) {
        checkStackUnderflow(count);
        for (size_t i = 0; i < count; ++i) {
            stack.pop_back();
        }
    }

    size_t StackManager::size() const {
        return stack.size();
    }

    bool StackManager::empty() const {
        return stack.empty();
    }

    void StackManager::clear() {
        stack.clear();
    }

    void StackManager::reserve(size_t capacity) {
        stack.reserve(capacity);
    }

    value::Value& StackManager::operator[](size_t index) {
        return stack[index];
    }

    const value::Value& StackManager::operator[](size_t index) const {
        return stack[index];
    }

    std::vector<value::Value>& StackManager::getStack() {
        return stack;
    }

    const std::vector<value::Value>& StackManager::getStack() const {
        return stack;
    }

    void StackManager::setStack(const std::vector<value::Value>& newStack) {
        stack = newStack;
    }

    void StackManager::resize(size_t newSize) {
        stack.resize(newSize);
    }

    void StackManager::resize(size_t newSize, const value::Value& defaultValue) {
        stack.resize(newSize, defaultValue);
    }

    void StackManager::checkStackUnderflow(size_t required) const {
        if (stack.size() < required) {
            throw errors::RuntimeException("Stack underflow: required " +
                std::to_string(required) + " values but stack has " +
                std::to_string(stack.size()));
        }
    }
}
