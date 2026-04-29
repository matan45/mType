#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "../../value/ValueShim.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/NativeContext.hpp"
#include "../../environment/registry/NativeRegistry.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../value/StringPool.hpp"
#include <vector>
#include <sstream>

namespace vm::jit
{
    static void performGenericBinop(value::Value* result,
                                     const value::Value* left,
                                     const value::Value* right,
                                     char op)
    {
        if (value::isInt(*left) && value::isInt(*right))
        {
            int64_t l = value::asInt(*left);
            int64_t r = value::asInt(*right);
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

        bool leftIsNumeric = value::isFloat(*left) || value::isInt(*left);
        bool rightIsNumeric = value::isFloat(*right) || value::isInt(*right);

        if (leftIsNumeric && rightIsNumeric)
        {
            double l = value::isFloat(*left)
                ? value::asFloat(*left)
                : static_cast<double>(value::asInt(*left));
            double r = value::isFloat(*right)
                ? value::asFloat(*right)
                : static_cast<double>(value::asInt(*right));

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

    // Phase 2: resolved dispatch — callers supply the pre-resolved JitFunction
    // and an already-interned frame-name handle, skipping both the name
    // hashmap lookup and the per-call internFrameName hash. tryJitDispatch
    // below is kept as the name-keyed fallback for CALL / deopt paths.
    static bool tryJitDispatchResolved(JitContext* ctx,
                                        JitFunction jitFn,
                                        bytecode::FunctionNameHandle frameName,
                                        size_t argCount)
    {
        if (!jitFn || !ctx->vm)
            return false;

        // If native recursion is too deep, fall back to interpreter to avoid
        // native C++ stack overflow (the VM interpreter uses a managed call stack)
        if (ctx->vm->getJitNativeDepth() >= vm::runtime::VirtualMachine::MAX_JIT_NATIVE_DEPTH)
            return false;

        vm::runtime::CallFrame frame;
        frame.returnAddress = 0;
        frame.frameBase = 0;
        frame.localBase = 0;
        frame.functionName = frameName;
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

    static bool tryJitDispatch(JitContext* ctx,
                               const std::string& funcName,
                               size_t argCount)
    {
        if (!ctx->jitCodeCache || !ctx->vm)
            return false;

        auto jitFn = ctx->jitCodeCache->lookup(funcName);
        if (!jitFn)
            return false;

        // MYT-197: intern on ctx->program (owner of the JIT-dispatched function).
        auto frameName = ctx->program->internFrameName(funcName);
        return tryJitDispatchResolved(ctx, jitFn, frameName, argCount);
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

        environment::NativeContext nativeCtx{ ctx->vm->getEnvironment(), ctx->vm->shared_from_this() };
        value::Value result = nativeFn(nativeCtx, std::span<const value::Value>(ctx->callArgs, argCount));
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
            // Phase 2 fast path: vector-indexed lookup in JitCodeCache. On
            // a hit we already have the JitFunction and a pre-interned frame
            // name — skip the name hashmap and per-call internFrameName.
            // This is the dominant path for recursive self-calls inside
            // compiled functions (e.g. fib/ack/gcd in recursive.mt).
            if (ctx->jitCodeCache)
            {
                auto cached = ctx->jitCodeCache->lookupByIndex(funcIndex);
                if (cached.fn)
                {
                    if (tryJitDispatchResolved(ctx, cached.fn,
                                                cached.frameName, argCount))
                        return;
                    // Falls through to slow path only when dispatch was
                    // rejected by the JitNativeDepth guard.
                }
            }

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

    void jit_throw_stack_overflow(size_t maxDepth)
    {
        std::ostringstream oss;
        oss << "Stack overflow: Maximum call stack depth of "
            << maxDepth << " exceeded.\n"
            << "This may indicate infinite recursion (JIT tail-call loop).";
        throw errors::RuntimeException(oss.str());
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

