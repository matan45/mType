#include "BitwiseExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"

namespace vm::runtime
{
    BitwiseExecutor::BitwiseExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void BitwiseExecutor::handleBitwiseAnd() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: BITWISE_AND requires 2 values");
        }
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        if (!std::holds_alternative<int64_t>(left) || !std::holds_alternative<int64_t>(right)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Bitwise AND requires integer operands");
        }

        int64_t l = std::get<int64_t>(left);
        int64_t r = std::get<int64_t>(right);
        context.stackManager->push(l & r);
    }

    void BitwiseExecutor::handleBitwiseOr() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: BITWISE_OR requires 2 values");
        }
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        if (!std::holds_alternative<int64_t>(left) || !std::holds_alternative<int64_t>(right)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Bitwise OR requires integer operands");
        }

        int64_t l = std::get<int64_t>(left);
        int64_t r = std::get<int64_t>(right);
        context.stackManager->push(l | r);
    }

    void BitwiseExecutor::handleBitwiseXor() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: BITWISE_XOR requires 2 values");
        }
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        if (!std::holds_alternative<int64_t>(left) || !std::holds_alternative<int64_t>(right)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Bitwise XOR requires integer operands");
        }

        int64_t l = std::get<int64_t>(left);
        int64_t r = std::get<int64_t>(right);
        context.stackManager->push(l ^ r);
    }

    void BitwiseExecutor::handleLeftShift() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: LEFT_SHIFT requires 2 values");
        }
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        if (!std::holds_alternative<int64_t>(left) || !std::holds_alternative<int64_t>(right)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Left shift requires integer operands");
        }

        int64_t l = std::get<int64_t>(left);
        int64_t r = std::get<int64_t>(right);

        // Validate shift amount is in valid range (0-63 for 64-bit integers)
        if (r < 0 || r > 63) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Shift amount must be between 0 and 63");
        }

        context.stackManager->push(l << r);
    }

    void BitwiseExecutor::handleRightShift() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: RIGHT_SHIFT requires 2 values");
        }
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        if (!std::holds_alternative<int64_t>(left) || !std::holds_alternative<int64_t>(right)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Right shift requires integer operands");
        }

        int64_t l = std::get<int64_t>(left);
        int64_t r = std::get<int64_t>(right);

        // Validate shift amount is in valid range (0-63 for 64-bit integers)
        if (r < 0 || r > 63) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Shift amount must be between 0 and 63");
        }

        context.stackManager->push(l >> r);
    }

    void BitwiseExecutor::handleBitwiseNot() {
        if (context.stackManager->size() < 1) {
            throw errors::RuntimeException("Stack underflow: BITWISE_NOT requires 1 value");
        }
        value::Value val = context.stackManager->pop();

        if (!std::holds_alternative<int64_t>(val)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Bitwise NOT requires integer operand");
        }

        int64_t v = std::get<int64_t>(val);
        context.stackManager->push(~v);
    }
}
