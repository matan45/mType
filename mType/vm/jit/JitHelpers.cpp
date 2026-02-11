#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../gc/GC.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/NativeRegistry.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include <vector>

namespace vm::jit
{
    extern "C" {

        // --- Phase 2: Int/Bool support ---

        int64_t jit_unbox_int(const value::Value* val)
        {
            if (std::holds_alternative<int64_t>(*val))
            {
                return std::get<int64_t>(*val);
            }
            if (std::holds_alternative<bool>(*val))
            {
                return std::get<bool>(*val) ? 1 : 0;
            }
            return 0;
        }

        void jit_set_return_int(JitContext* ctx, int64_t val)
        {
            ctx->returnValue = val;
            ctx->hasReturnValue = true;
        }

        void jit_set_return_bool(JitContext* ctx, int64_t val)
        {
            ctx->returnValue = (val != 0);
            ctx->hasReturnValue = true;
        }

        void jit_gc_safepoint()
        {
            gc::GC::maybeCollect();
        }

        // --- Phase 3: Float support ---

        float jit_unbox_float(const value::Value* val)
        {
            if (std::holds_alternative<float>(*val))
            {
                return std::get<float>(*val);
            }
            if (std::holds_alternative<int64_t>(*val))
            {
                return static_cast<float>(std::get<int64_t>(*val));
            }
            return 0.0f;
        }

        void jit_set_return_float(JitContext* ctx, float val)
        {
            ctx->returnValue = val;
            ctx->hasReturnValue = true;
        }

        // --- Phase 3: Boxing helpers ---

        void jit_box_int(value::Value* dest, int64_t val)
        {
            *dest = val;
        }

        void jit_box_float(value::Value* dest, float val)
        {
            *dest = val;
        }

        void jit_box_bool(value::Value* dest, int64_t val)
        {
            *dest = (val != 0);
        }

        void jit_box_null(value::Value* dest)
        {
            *dest = std::monostate{};
        }

    } // extern "C"

    // --- Phase 3: Call dispatch ---
    // Outside extern "C" because these throw C++ exceptions.

    static void performGenericBinop(value::Value* result,
                                     const value::Value* left,
                                     const value::Value* right,
                                     char op)
    {
        // Integer + Integer
        if (std::holds_alternative<int64_t>(*left) && std::holds_alternative<int64_t>(*right))
        {
            int64_t l = std::get<int64_t>(*left);
            int64_t r = std::get<int64_t>(*right);
            switch (op)
            {
                case '+': *result = l + r; return;
                case '-': *result = l - r; return;
                case '*': *result = l * r; return;
                case '/':
                    if (r == 0) throw errors::RuntimeException("Division by zero");
                    *result = l / r; return;
                case '%':
                    if (r == 0) throw errors::RuntimeException("Modulo by zero");
                    *result = l % r; return;
            }
        }

        // Float + Float or Int + Float or Float + Int
        bool leftIsNumeric = std::holds_alternative<float>(*left) || std::holds_alternative<int64_t>(*left);
        bool rightIsNumeric = std::holds_alternative<float>(*right) || std::holds_alternative<int64_t>(*right);

        if (leftIsNumeric && rightIsNumeric)
        {
            float l = std::holds_alternative<float>(*left)
                ? std::get<float>(*left)
                : static_cast<float>(std::get<int64_t>(*left));
            float r = std::holds_alternative<float>(*right)
                ? std::get<float>(*right)
                : static_cast<float>(std::get<int64_t>(*right));

            switch (op)
            {
                case '+': *result = l + r; return;
                case '-': *result = l - r; return;
                case '*': *result = l * r; return;
                case '/':
                    if (r == 0.0f) throw errors::RuntimeException("Division by zero");
                    *result = l / r; return;
                case '%':
                    throw errors::RuntimeException("Modulo not supported for float");
            }
        }

        throw errors::RuntimeException("Unsupported operand types for arithmetic");
    }

    void jit_call_function(JitContext* ctx, uint32_t nameIndex, size_t argCount)
    {
        const std::string& funcName = ctx->program->getConstantPool().getString(nameIndex);

        // 1. Check JIT cache - JIT->JIT direct call
        if (ctx->jitCodeCache)
        {
            auto jitFn = ctx->jitCodeCache->lookup(funcName);
            if (jitFn)
            {
                JitContext nestedCtx{};
                nestedCtx.args = ctx->callArgs;
                nestedCtx.argCount = argCount;
                nestedCtx.hasReturnValue = false;
                nestedCtx.program = ctx->program;
                nestedCtx.stackManager = ctx->stackManager;
                nestedCtx.environment = ctx->environment;
                nestedCtx.vm = ctx->vm;
                nestedCtx.jitCodeCache = ctx->jitCodeCache;

                jitFn(&nestedCtx);

                ctx->returnValue = nestedCtx.returnValue;
                ctx->hasReturnValue = nestedCtx.hasReturnValue;
                return;
            }
        }

        // 2. Check native registry
        auto nativeRegistry = ctx->environment->getNativeRegistry();
        if (nativeRegistry && nativeRegistry->hasNativeFunction(funcName))
        {
            auto nativeFn = nativeRegistry->findNativeFunction(funcName);
            if (nativeFn)
            {
                std::vector<value::Value> argVec(ctx->callArgs, ctx->callArgs + argCount);
                value::Value result = nativeFn(argVec);
                ctx->returnValue = result;
                ctx->hasReturnValue = true;
                return;
            }
        }

        // 3. Fall back to interpreter
        if (ctx->vm)
        {
            std::vector<value::Value> argVec(ctx->callArgs, ctx->callArgs + argCount);
            ctx->returnValue = ctx->vm->callFunctionFromJit(funcName, argVec);
            ctx->hasReturnValue = true;
            return;
        }

        throw errors::RuntimeException("JIT: cannot call function '" + funcName + "'");
    }

    // --- Phase 3: Generic arithmetic helpers ---

    void jit_generic_add(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '+');
    }

    void jit_generic_sub(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '-');
    }

    void jit_generic_mul(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '*');
    }

    void jit_generic_div(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '/');
    }

    void jit_generic_mod(value::Value* result, const value::Value* left, const value::Value* right)
    {
        performGenericBinop(result, left, right, '%');
    }

    void jit_throw_div_by_zero()
    {
        throw errors::RuntimeException("Division by zero");
    }
}
