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

        bool leftIsNumeric = std::holds_alternative<double>(*left) || std::holds_alternative<int64_t>(*left);
        bool rightIsNumeric = std::holds_alternative<double>(*right) || std::holds_alternative<int64_t>(*right);

        if (leftIsNumeric && rightIsNumeric)
        {
            double l = std::holds_alternative<double>(*left)
                ? std::get<double>(*left)
                : static_cast<double>(std::get<int64_t>(*left));
            double r = std::holds_alternative<double>(*right)
                ? std::get<double>(*right)
                : static_cast<double>(std::get<int64_t>(*right));

            switch (op)
            {
                case '+': *result = l + r; return;
                case '-': *result = l - r; return;
                case '*': *result = l * r; return;
                case '/':
                    if (r == 0.0) throw errors::RuntimeException("Division by zero");
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
        if (!ctx->jitCodeCache || !ctx->vm)
            return false;

        auto jitFn = ctx->jitCodeCache->lookup(funcName);
        if (!jitFn)
            return false;

        // If native recursion is too deep, fall back to interpreter to avoid
        // native C++ stack overflow (the VM interpreter uses a managed call stack)
        if (ctx->vm->getJitNativeDepth() >= vm::runtime::VirtualMachine::MAX_JIT_NATIVE_DEPTH)
            return false;

        // Track on the VM call stack for overflow protection
        vm::runtime::CallFrame frame;
        frame.returnAddress = 0;
        frame.frameBase = 0;
        frame.localBase = 0;
        frame.functionName = funcName;
        frame.thisInstance = nullptr;
        ctx->vm->pushCallFrame(frame);
        ctx->vm->incrementJitNativeDepth();

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

        ctx->vm->decrementJitNativeDepth();
        ctx->vm->popCallStack();

        // Propagate any exception stored by JIT helpers
        if (nestedCtx.pendingException)
        {
            ctx->pendingException = nestedCtx.pendingException;
            return true;  // Return true so caller doesn't try other dispatch paths
        }

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
        // If a previous call in this JIT frame already failed, bail out immediately
        if (ctx->pendingException)
            return;

        try
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
        catch (...)
        {
            // Store exception — don't let it unwind through JIT-generated frames
            ctx->pendingException = std::current_exception();
        }
    }

    void jit_call_function_fast(JitContext* ctx, uint32_t funcIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
            const auto* meta = ctx->program->getFunctionByIndex(funcIndex);
            if (!meta)
                throw errors::RuntimeException(
                    "JIT: invalid function index " + std::to_string(funcIndex));

            // CALL_FAST only targets non-native user-defined bytecode functions
            // (guaranteed by FunctionCallHelper.cpp emission conditions), so the
            // mangled name is the authoritative dispatch key — matches the
            // lookup done by VirtualMachine::executeCallFastWithJit.
            const std::string& funcName =
                meta->mangledName.empty() ? meta->name : meta->mangledName;

            if (tryJitDispatch(ctx, funcName, argCount))
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
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
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

    void jit_throw_shift_out_of_range(int64_t /*count*/)
    {
        throw errors::RuntimeException("Shift amount must be between 0 and 63");
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
