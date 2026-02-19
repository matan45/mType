#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/NativeRegistry.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../value/StringPool.hpp"
#include <vector>

namespace vm::jit
{
    static void performGenericBinop(value::Value* result,
                                     const value::Value* left,
                                     const value::Value* right,
                                     char op)
    {
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

    static bool tryJitDispatch(JitContext* ctx,
                               const std::string& funcName,
                               size_t argCount)
    {
        if (!ctx->jitCodeCache)
            return false;

        auto jitFn = ctx->jitCodeCache->lookup(funcName);
        if (!jitFn)
            return false;

        JitContext nestedCtx{};
        nestedCtx.args = ctx->callArgs;
        nestedCtx.argCount = argCount;
        nestedCtx.hasReturnValue = false;
        nestedCtx.program = ctx->program;
        nestedCtx.stackManager = ctx->stackManager;
        nestedCtx.environment = ctx->environment;
        nestedCtx.vm = ctx->vm;
        nestedCtx.jitCodeCache = ctx->jitCodeCache;
        nestedCtx.icTable = ctx->icTable;
        nestedCtx.callingClassName = ctx->callingClassName;

        jitFn(&nestedCtx);

        ctx->returnValue = nestedCtx.returnValue;
        ctx->hasReturnValue = nestedCtx.hasReturnValue;
        return true;
    }

    static bool tryNativeDispatch(JitContext* ctx,
                                  const std::string& funcName,
                                  size_t argCount)
    {
        auto nativeRegistry = ctx->environment->getNativeRegistry();
        if (!nativeRegistry || !nativeRegistry->hasNativeFunction(funcName))
            return false;

        auto nativeFn = nativeRegistry->findNativeFunction(funcName);
        if (!nativeFn)
            return false;

        std::vector<value::Value> argVec(ctx->callArgs, ctx->callArgs + argCount);
        value::Value result = nativeFn(argVec);
        ctx->returnValue = result;
        ctx->hasReturnValue = true;
        return true;
    }

    void jit_call_function(JitContext* ctx, uint32_t nameIndex, size_t argCount)
    {
        const std::string& funcName = ctx->program->getConstantPool().getString(nameIndex);

        if (tryJitDispatch(ctx, funcName, argCount))
            return;

        if (tryNativeDispatch(ctx, funcName, argCount))
            return;

        if (ctx->vm)
        {
            std::vector<value::Value> argVec(ctx->callArgs, ctx->callArgs + argCount);
            ctx->returnValue = ctx->vm->callFunctionFromJit(funcName, argVec);
            ctx->hasReturnValue = true;
            return;
        }

        throw errors::RuntimeException("JIT: cannot call function '" + funcName + "'");
    }

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

    void jit_value_copy(value::Value* dest, const value::Value* src)
    {
        *dest = *src;
    }

    void jit_set_return_boxed(JitContext* ctx, const value::Value* val)
    {
        ctx->returnValue = *val;
        ctx->hasReturnValue = true;
    }

    void jit_value_swap(value::Value* a, value::Value* b)
    {
        value::Value temp = std::move(*a);
        *a = std::move(*b);
        *b = std::move(temp);
    }

    void jit_push_string(value::Value* dest,
                          const vm::bytecode::BytecodeProgram* prog,
                          uint32_t constIndex)
    {
        const std::string& str = prog->getConstantPool().getString(constIndex);
        auto& pool = value::StringPool::getInstance();
        *dest = pool.intern(str);
    }
}
