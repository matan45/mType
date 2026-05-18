#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include "../context/ExecutionContext.hpp"
#include "../../../environment/Environment.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/IntegerCache.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../utils/ErrorLocationHelper.hpp"

namespace vm::runtime {

/**
 * Phase 3: Executor for optimized primitive method calls
 *
 * Handles specialized INVOKE_INT_* / INVOKE_FLOAT_* / INVOKE_BOOL_* /
 * INVOKE_STRING_* opcodes.
 *
 * Pattern for each handler:
 * 1. Pop arguments and receiver from stack
 * 2. Unbox: Extract primitive values from object fields (helper in .cpp)
 * 3. Fast operation: Direct arithmetic/comparison
 * 4. Push raw primitive result (lazy re-boxing)
 *
 * MYT-319: all 31 INVOKE_* handlers are inlined here so MSVC v145 (no /GL)
 * can inline them through the dispatch switch's unique_ptr deref. The
 * unbox{Int,Float,Bool,String}FromValue helpers and the boxing helpers
 * stay out-of-line in PrimitiveMethodExecutor.cpp — they pull in
 * ObjectInstancePool / ValueObject / StringPool / ClassDefinition.
 */
class PrimitiveMethodExecutor {
public:
    PrimitiveMethodExecutor(ExecutionContext& ctx,
                            std::shared_ptr<environment::Environment> env)
        : context(ctx)
        , environment(std::move(env))
        , cachedIntClass_(nullptr)
        , cachedFloatClass_(nullptr)
    {}
    ~PrimitiveMethodExecutor() = default;

    // === Int Object Method Handlers ===

    inline void handleInvokeIntAdd() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);

        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);

        // Lazy re-boxing: push raw primitive, only box at escape points
        context.stackManager->push(receiver + arg);
    }

    inline void handleInvokeIntSub() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push(receiver - arg);
    }

    inline void handleInvokeIntMul() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push(receiver * arg);
    }

    inline void handleInvokeIntDiv() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);

        if (arg == 0) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero in Int.divide()");
        }

        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push(receiver / arg);
    }

    inline void handleInvokeIntMod() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);

        if (arg == 0) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Modulo by zero in Int.modulo()");
        }

        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push(receiver % arg);
    }

    inline void handleInvokeIntNeg() {
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push(-receiver);
    }

    inline void handleInvokeIntAbs() {
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push((receiver < 0) ? -receiver : receiver);
    }

    inline void handleInvokeIntEquals() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        bool result = (receiver == arg);
        context.stackManager->push(result);
    }

    inline void handleInvokeIntCompare() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        int result = (receiver < arg) ? -1 : (receiver > arg) ? 1 : 0;
        context.stackManager->push(result);
    }

    // Inline accessors (MYT-155).
    inline void handleInvokeIntGetValue() {
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push(receiver);
    }

    inline void handleInvokeFloatGetValue() {
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        context.stackManager->push(receiver);
    }

    void handleInvokeBoolGetValue();  // Out-of-line: ValueObject + 3 branches, less hot.

    // Inline comparison ops (MYT-155).
    inline void handleInvokeIntLessThan() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push(receiver < arg);
    }

    inline void handleInvokeIntLessEqual() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push(receiver <= arg);
    }

    inline void handleInvokeIntGreaterThan() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push(receiver > arg);
    }

    inline void handleInvokeIntGreaterEqual() {
        value::Value argValue = context.stackManager->pop();
        int64_t arg = unboxIntFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        int64_t receiver = unboxIntFromValue(receiverValue);
        context.stackManager->push(receiver >= arg);
    }

    inline void handleInvokeFloatLessThan() {
        value::Value argValue = context.stackManager->pop();
        double arg = unboxFloatFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        context.stackManager->push(receiver < arg);
    }

    inline void handleInvokeFloatLessEqual() {
        value::Value argValue = context.stackManager->pop();
        double arg = unboxFloatFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        context.stackManager->push(receiver <= arg);
    }

    inline void handleInvokeFloatGreaterThan() {
        value::Value argValue = context.stackManager->pop();
        double arg = unboxFloatFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        context.stackManager->push(receiver > arg);
    }

    inline void handleInvokeFloatGreaterEqual() {
        value::Value argValue = context.stackManager->pop();
        double arg = unboxFloatFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        context.stackManager->push(receiver >= arg);
    }

    // === Float Object Method Handlers ===

    inline void handleInvokeFloatAdd() {
        value::Value argValue = context.stackManager->pop();
        double arg = unboxFloatFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        // Lazy re-boxing: push raw primitive
        context.stackManager->push(receiver + arg);
    }

    inline void handleInvokeFloatSub() {
        value::Value argValue = context.stackManager->pop();
        double arg = unboxFloatFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        context.stackManager->push(receiver - arg);
    }

    inline void handleInvokeFloatMul() {
        value::Value argValue = context.stackManager->pop();
        double arg = unboxFloatFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        context.stackManager->push(receiver * arg);
    }

    inline void handleInvokeFloatDiv() {
        value::Value argValue = context.stackManager->pop();
        double arg = unboxFloatFromValue(argValue);

        if (arg == 0.0) {
            utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero in Float.divide()");
        }

        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        context.stackManager->push(receiver / arg);
    }

    inline void handleInvokeFloatNeg() {
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        context.stackManager->push(-receiver);
    }

    inline void handleInvokeFloatAbs() {
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        context.stackManager->push((receiver < 0.0) ? -receiver : receiver);
    }

    inline void handleInvokeFloatEquals() {
        value::Value argValue = context.stackManager->pop();
        double arg = unboxFloatFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        bool result = (receiver == arg);
        context.stackManager->push(result);
    }

    inline void handleInvokeFloatCompare() {
        value::Value argValue = context.stackManager->pop();
        double arg = unboxFloatFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        double receiver = unboxFloatFromValue(receiverValue);
        int64_t result = (receiver < arg) ? -1 : (receiver > arg) ? 1 : 0;
        context.stackManager->push(result);
    }

    // === Bool Object Method Handlers ===

    inline void handleInvokeBoolAnd() {
        value::Value argValue = context.stackManager->pop();
        bool arg = unboxBoolFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        bool receiver = unboxBoolFromValue(receiverValue);
        context.stackManager->push(value::Value(receiver && arg));
    }

    inline void handleInvokeBoolOr() {
        value::Value argValue = context.stackManager->pop();
        bool arg = unboxBoolFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        bool receiver = unboxBoolFromValue(receiverValue);
        context.stackManager->push(value::Value(receiver || arg));
    }

    inline void handleInvokeBoolXor() {
        value::Value argValue = context.stackManager->pop();
        bool arg = unboxBoolFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        bool receiver = unboxBoolFromValue(receiverValue);
        // Mirror Bool.xor: (a||b) && !(a&&b), equivalent to a != b for bools.
        context.stackManager->push(value::Value(receiver != arg));
    }

    inline void handleInvokeBoolNot() {
        value::Value receiverValue = context.stackManager->pop();
        bool receiver = unboxBoolFromValue(receiverValue);
        context.stackManager->push(value::Value(!receiver));
    }

    inline void handleInvokeBoolEquals() {
        value::Value argValue = context.stackManager->pop();
        bool arg = unboxBoolFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        bool receiver = unboxBoolFromValue(receiverValue);
        context.stackManager->push(value::Value(receiver == arg));
    }

    // === String Object Method Handlers ===

    inline void handleInvokeStringLength() {
        value::Value receiverValue = context.stackManager->pop();
        std::string s = unboxStringFromValue(receiverValue);
        context.stackManager->push(static_cast<int64_t>(s.size()));
    }

    // String concat needs StringPool, which is a heavier include. Keep .cpp.
    void handleInvokeStringConcat();

    inline void handleInvokeStringEquals() {
        value::Value argValue = context.stackManager->pop();
        std::string arg = unboxStringFromValue(argValue);
        value::Value receiverValue = context.stackManager->pop();
        std::string receiver = unboxStringFromValue(receiverValue);
        context.stackManager->push(value::Value(receiver == arg));
    }

    inline void handleInvokeStringIsEmpty() {
        value::Value receiverValue = context.stackManager->pop();
        std::string s = unboxStringFromValue(receiverValue);
        context.stackManager->push(value::Value(s.empty()));
    }

private:
    ExecutionContext& context;
    std::shared_ptr<environment::Environment> environment;

    // === Helper Methods — out-of-line in .cpp (ObjectInstancePool / ValueObject / StringPool / ClassDefinition) ===

    int64_t unboxInt(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj);
    double unboxFloat(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj);

    // Value-based unbox: handles both ObjectInstance and ValueObject
    int64_t unboxIntFromValue(const value::Value& val);
    double unboxFloatFromValue(const value::Value& val);
    bool unboxBoolFromValue(const value::Value& val);
    std::string unboxStringFromValue(const value::Value& val);

    // Box that returns appropriate type (ValueObject if value class, ObjectInstance otherwise)
    value::Value boxIntValue(int64_t val);
    value::Value boxFloatValue(double val);

    // Legacy box methods (always return ObjectInstance)
    std::shared_ptr<runtimeTypes::klass::ObjectInstance> boxInt(int64_t value);
    std::shared_ptr<runtimeTypes::klass::ObjectInstance> boxFloat(double value);

    std::shared_ptr<runtimeTypes::klass::ClassDefinition> getIntClass();
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> getFloatClass();

    // Cached class definitions (lazy initialized)
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> cachedIntClass_;
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> cachedFloatClass_;
};

} // namespace vm::runtime
