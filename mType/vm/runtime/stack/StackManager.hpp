#pragma once
#include <vector>
#include <cstddef>
#include <utility>
#include "../../../value/ValueType.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    /**
     * Manages the operand stack for the virtual machine
     * Provides stack operations with underflow validation
     *
     * MYT-318: hot methods (push, pop, peek, operator[], size, empty) live
     * inline in this header so executors can be optimized via inlining +
     * dead-store elimination across pop/pop/push chains. The replaceTop /
     * binaryReplaceTop helpers collapse common arithmetic patterns into a
     * single vector mutation (the prior pop/pop/push path touched the size
     * field three times per binary op).
     */
    class StackManager
    {
    public:
        StackManager() { stack.reserve(256); }
        ~StackManager() = default;

        // === Stack operations (inline hot path) ===
        inline void push(const value::Value& value) {
            stack.push_back(value);
        }
        inline void push(value::Value&& value) {
            stack.push_back(std::move(value));
        }

        inline value::Value pop() {
            checkStackUnderflow(1);
            value::Value val = std::move(stack.back());
            stack.pop_back();
            return val;
        }

        inline value::Value peek(size_t offset = 0) const {
            checkStackUnderflow(offset + 1);
            return stack[stack.size() - 1 - offset];
        }

        // Reference to a stack slot relative to TOS, without bounds check on
        // hot paths (caller has already established the depth invariant from
        // the bytecode contract). Reference remains valid only until the next
        // push / pop / resize on this StackManager.
        inline value::Value& peekRef(size_t offset = 0) {
            return stack[stack.size() - 1 - offset];
        }
        inline const value::Value& peekRef(size_t offset = 0) const {
            return stack[stack.size() - 1 - offset];
        }

        // Overwrite the top-of-stack in place. Equivalent to `pop(); push(v);`
        // but touches the vector size field zero times. Used by unary ops
        // (handleNeg, handleInc, handleDec) where the result replaces the
        // single popped operand.
        inline void replaceTop(value::Value v) {
            checkStackUnderflow(1);
            stack.back() = std::move(v);
        }

        // Pop the topmost element and overwrite the new top in place.
        // Equivalent to `pop(); pop(); push(v);` but touches the vector size
        // once instead of three times. Used by binary ops (handleAdd, ...).
        inline void binaryReplaceTop(value::Value v) {
            checkStackUnderflow(2);
            stack.pop_back();
            stack.back() = std::move(v);
        }

        inline void popN(size_t count) {
            checkStackUnderflow(count);
            for (size_t i = 0; i < count; ++i) {
                stack.pop_back();
            }
        }

        // === Stack state (inline) ===
        inline size_t size() const { return stack.size(); }
        inline bool empty() const { return stack.empty(); }
        inline void clear() { stack.clear(); }
        inline void reserve(size_t capacity) { stack.reserve(capacity); }

        // === Direct access ===
        inline value::Value& operator[](size_t index) { return stack[index]; }
        inline const value::Value& operator[](size_t index) const { return stack[index]; }

        inline std::vector<value::Value>& getStack() { return stack; }
        inline const std::vector<value::Value>& getStack() const { return stack; }
        inline void setStack(const std::vector<value::Value>& newStack) { stack = newStack; }

        // Stack size manipulation (for call frames) — out of line; cold path.
        void resize(size_t newSize);
        void resize(size_t newSize, const value::Value& defaultValue);

    private:
        std::vector<value::Value> stack;

        inline void checkStackUnderflow(size_t required) const {
            if (stack.size() < required) {
                throwStackUnderflow(required);
            }
        }

        // Out-of-line so the inline checkStackUnderflow stays tiny and the
        // exception construction (string formatting) doesn't bloat call sites.
        [[noreturn]] void throwStackUnderflow(size_t required) const;
    };
}
