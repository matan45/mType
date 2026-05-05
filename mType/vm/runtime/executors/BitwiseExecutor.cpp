#include "BitwiseExecutor.hpp"
#include <cstdint>
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../value/ValueShim.hpp"

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

        if (!value::isInt(left) || !value::isInt(right)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Bitwise AND requires integer operands");
        }

        int64_t l = value::asInt(left);
        int64_t r = value::asInt(right);
        context.stackManager->push(l & r);
    }

    void BitwiseExecutor::handleBitwiseOr() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: BITWISE_OR requires 2 values");
        }
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        if (!value::isInt(left) || !value::isInt(right)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Bitwise OR requires integer operands");
        }

        int64_t l = value::asInt(left);
        int64_t r = value::asInt(right);
        context.stackManager->push(l | r);
    }

    void BitwiseExecutor::handleBitwiseXor() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: BITWISE_XOR requires 2 values");
        }
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        if (!value::isInt(left) || !value::isInt(right)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Bitwise XOR requires integer operands");
        }

        int64_t l = value::asInt(left);
        int64_t r = value::asInt(right);
        context.stackManager->push(l ^ r);
    }

    void BitwiseExecutor::handleLeftShift() {
        if (context.stackManager->size() < 2) {
            throw errors::RuntimeException("Stack underflow: LEFT_SHIFT requires 2 values");
        }
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        if (!value::isInt(left) || !value::isInt(right)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Left shift requires integer operands");
        }

        int64_t l = value::asInt(left);
        int64_t r = value::asInt(right);

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

        if (!value::isInt(left) || !value::isInt(right)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Right shift requires integer operands");
        }

        int64_t l = value::asInt(left);
        int64_t r = value::asInt(right);

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

        if (!value::isInt(val)) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Bitwise NOT requires integer operand");
        }

        int64_t v = value::asInt(val);
        context.stackManager->push(~v);
    }

    // Type-specialized INT variants. Preconditions (established at runtime by
    // trySpecializeBitwise once the operand types are observed monomorphic):
    // operands should be INT. Each still includes a type guard that falls
    // back to the generic handler on miss — mirrors the handleAddInt pattern
    // so polymorphic call sites stay correct even after promotion.
    void BitwiseExecutor::handleBitwiseAndInt() {
        auto& stack = context.stackManager->getStack();
        const value::Value& r = stack.back();
        const value::Value& l = stack[stack.size() - 2];
        if (!value::isInt(l) || !value::isInt(r)) { handleBitwiseAnd(); return; }
        int64_t rv = r.rawInt(); int64_t lv = l.rawInt();
        stack.pop_back();
        stack.back() = value::Value(lv & rv);
    }

    void BitwiseExecutor::handleBitwiseOrInt() {
        auto& stack = context.stackManager->getStack();
        const value::Value& r = stack.back();
        const value::Value& l = stack[stack.size() - 2];
        if (!value::isInt(l) || !value::isInt(r)) { handleBitwiseOr(); return; }
        int64_t rv = r.rawInt(); int64_t lv = l.rawInt();
        stack.pop_back();
        stack.back() = value::Value(lv | rv);
    }

    void BitwiseExecutor::handleBitwiseXorInt() {
        auto& stack = context.stackManager->getStack();
        const value::Value& r = stack.back();
        const value::Value& l = stack[stack.size() - 2];
        if (!value::isInt(l) || !value::isInt(r)) { handleBitwiseXor(); return; }
        int64_t rv = r.rawInt(); int64_t lv = l.rawInt();
        stack.pop_back();
        stack.back() = value::Value(lv ^ rv);
    }

    void BitwiseExecutor::handleLeftShiftInt() {
        auto& stack = context.stackManager->getStack();
        const value::Value& r = stack.back();
        const value::Value& l = stack[stack.size() - 2];
        if (!value::isInt(l) || !value::isInt(r)) { handleLeftShift(); return; }
        int64_t rv = r.rawInt(); int64_t lv = l.rawInt();
        if (rv < 0 || rv > 63) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Shift amount must be between 0 and 63");
        }
        stack.pop_back();
        stack.back() = value::Value(lv << rv);
    }

    void BitwiseExecutor::handleRightShiftInt() {
        auto& stack = context.stackManager->getStack();
        const value::Value& r = stack.back();
        const value::Value& l = stack[stack.size() - 2];
        if (!value::isInt(l) || !value::isInt(r)) { handleRightShift(); return; }
        int64_t rv = r.rawInt(); int64_t lv = l.rawInt();
        if (rv < 0 || rv > 63) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Shift amount must be between 0 and 63");
        }
        stack.pop_back();
        stack.back() = value::Value(lv >> rv);
    }

    void BitwiseExecutor::handleBitwiseNotInt() {
        auto& stack = context.stackManager->getStack();
        const value::Value& v = stack.back();
        if (!value::isInt(v)) { handleBitwiseNot(); return; }
        stack.back() = value::Value(~v.rawInt());
    }
}
