#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    /**
     * Executes bitwise opcodes
     * Handles BITWISE_AND, BITWISE_OR, BITWISE_XOR, LEFT_SHIFT, RIGHT_SHIFT, BITWISE_NOT
     */
    class BitwiseExecutor
    {
    public:
        explicit BitwiseExecutor(ExecutionContext& ctx);
        ~BitwiseExecutor() = default;

        // Binary bitwise operations
        void handleBitwiseAnd();    // &
        void handleBitwiseOr();     // |
        void handleBitwiseXor();    // ^
        void handleLeftShift();     // <<
        void handleRightShift();    // >>

        // Unary bitwise operation
        void handleBitwiseNot();    // ~

        // Type-specialized INT variants: compiler / trySpecializeBitwise has
        // already guaranteed both operands are INT, so these skip the
        // isInt/asInt dispatch and read the payload directly. Shift variants
        // still range-check the operand (runtime value, not static type).
        void handleBitwiseAndInt();
        void handleBitwiseOrInt();
        void handleBitwiseXorInt();
        void handleLeftShiftInt();
        void handleRightShiftInt();
        void handleBitwiseNotInt();

    private:
        ExecutionContext& context;
    };
}
