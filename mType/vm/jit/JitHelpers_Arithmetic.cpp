#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/ValueObject.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
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
    // MYT-254 follow-up: minimal valueToString for the JIT generic-binop
    // string-concat fallback. Mirrors ArithmeticExecutor::valueToString for
    // the primitive cases (INT/FLOAT/BOOL/STRING/INTERNED_STRING/NULL/VOID),
    // plus the Int/Float/Bool/String value-class wrappers via field-0 unbox
    // (matches tryReadBoxedField's "single `value` field at index 0"
    // invariant from the lib/primitives boxed types). Custom-toString
    // OBJECT receivers fall through to the throw path; the JIT can't
    // safely re-enter the interpreter for arbitrary toString() dispatch
    // from inside this helper without risking re-entrancy bugs.
    static bool valueToStringForConcat(const value::Value& v, std::string& out)
    {
        if (value::isInt(v))
        {
            out = std::to_string(value::asInt(v));
            return true;
        }
        if (value::isFloat(v))
        {
            std::ostringstream oss;
            oss << value::asFloat(v);
            out = oss.str();
            return true;
        }
        if (value::isBool(v))
        {
            out = value::asBool(v) ? "true" : "false";
            return true;
        }
        if (value::isString(v))
        {
            out = value::asString(v);
            return true;
        }
        if (value::isInternedString(v))
        {
            out = value::asInternedString(v).getString();
            return true;
        }
        if (value::isNullType(v))
        {
            out = "null";
            return true;
        }
        if (value::isVoid(v))
        {
            out = "void";
            return true;
        }
        // Boxed primitive wrappers (Int/Float/Bool/String value classes from
        // lib/primitives): single `value` field at index 0 holding the
        // underlying primitive. Recurse on the field — handles `text + i`
        // where i is `Int` and `prefix + flag` where flag is `Bool`.
        if (value::isValueObject(v))
        {
            const auto& vo = value::asValueObject(v);
            if (vo && vo->getFieldCount() > 0)
                return valueToStringForConcat(vo->getFieldByIndex(0), out);
            return false;
        }
        if (value::isAnyObject(v))
        {
            auto* obj = value::asObjectInstanceRaw(v);
            if (obj && obj->hasFieldVector())
                return valueToStringForConcat(obj->getFieldByIndex(0), out);
            return false;
        }
        return false;
    }

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

        // MYT-254: string concatenation for ADD with at least one string-like
        // operand. Mirrors ArithmeticExecutor::performBinaryOp's string-concat
        // path so JIT-compiled outer loops handle `"-" + (i % 13)` and similar
        // mixed-type ADD chains the same way the interpreter does. Without
        // this, the throw below was being caught into ctx->pendingException
        // and the JIT loop continued with garbage, producing corrupted
        // String VALUE_OBJECTs whose value field held the right operand's
        // raw int (boxed_primitive_dispatch_hot.mt regression).
        if (op == '+' &&
            (value::isString(*left) || value::isString(*right) ||
             value::isInternedString(*left) || value::isInternedString(*right)))
        {
            std::string ls, rs;
            if (valueToStringForConcat(*left, ls) &&
                valueToStringForConcat(*right, rs))
            {
                *result = value::StringPool::getInstance().intern(ls + rs);
                return;
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

        // Self-recursive async dispatch refusal. Async / boxed-mode bodies
        // allocate ~10KB of asmjit-stack per frame (256-slot stackBase +
        // 256-slot boxedBase × Value=32B + locals); the global MAX=256 is
        // calibrated for non-boxed (pure-primitive) frames at ~1KB. Even
        // ~80 levels of async self-recursion blow Windows' 1MB native stack
        // — and unlike a clean unwind via OSRDeoptException, a deep
        // pushCallFrame throw stashes through pendingException across each
        // JIT-from-JIT level whose asmjit body has no pending-exception
        // checks between opcodes (jit_has_pending_exception is only emitted
        // for OSR loops). The result is a silent process exit before the
        // managed callStack overflow message can print. Bailing async
        // self-recursive dispatch on the first level routes the recursion
        // through callFunctionFromJit's mini-interpret with the
        // inJitFallbackInterpreter flag, where managed callStack growth
        // hits maxCallStackSize=10000 and throws cleanly. fib/ack and
        // other non-async self-recursion stay fully JIT-compiled.
        const auto& cs = ctx->vm->getCallStack();
        if (!cs.empty() && cs.back().functionName == frameName)
        {
            const std::string& fname = ctx->program->getFrameName(frameName);
            const auto* meta = ctx->program->getFunction(fname);
            if (meta && meta->isAsync)
                return false;
        }

        vm::runtime::CallFrame frame;
        frame.returnAddress = 0;
        frame.frameBase = 0;
        frame.localBase = 0;
        frame.functionName = frameName;
        frame.thisInstance = nullptr;
        ctx->vm->pushCallFrame(std::move(frame));
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

        // MYT-268: jit_await stashes OSRDeoptException on
        // nestedCtx.pendingException instead of throwing (the throw form
        // crashed silently on Windows x64 — no PE unwind data registered
        // for the asmjit frame). The deopt now propagates via the
        // pendingException field below; the prior catch (OSRDeoptException&)
        // wrapper is unreachable.
        jitFn(&nestedCtx);
        ctx->vm->decrementJitNativeDepth();
        ctx->vm->popCallStack();

        // Propagate any exception stored by JIT helpers (including the
        // OSRDeoptException stashed by jit_await — the function-level
        // deopt boundary in executeCallWithJit detects it and routes to
        // the interpreter fallback).
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
                ctx->returnValue = ctx->vm->callFunctionFromJit(
                    funcName, std::span<const value::Value>(ctx->callArgs, argCount));
                ctx->hasReturnValue = true;
                return;
            }

            throw errors::RuntimeException("JIT: cannot call function '" + funcName + "'");
        }
        catch (...)
        {
            // Store exception — don't let it unwind through JIT-generated frames.
            // (Function-level JIT bodies have no implicit pendingException check
            // between opcodes; rely on each helper's entry-check to short-circuit
            // until the body returns and executeCallWithJit rethrows.)
            // MYT-268: includes the OSRDeoptException stashed by jit_await
            // (now propagated via nestedCtx.pendingException in
            // tryJitDispatchResolved, then assigned to ctx->pendingException
            // before the helper returns — the prior catch (OSRDeoptException&)
            // arm is unreachable since no callee throws this type anymore).
            ctx->pendingException = std::current_exception();
        }
    }

    void jit_call_function_ic(JitContext* ctx, size_t bytecodeOffset,
                              uint32_t nameIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
            const std::string& funcName = ctx->program->getConstantPool().getString(nameIndex);

            // IC slot lookup. On warm path we route based on what was probed
            // and cached on the first call at this IP. The JIT-dispatch arm
            // is split out into a "do we even attempt tryJitDispatchResolved"
            // gate (jitProbeDone) so a callee that's NOT separately JIT-
            // compiled never re-pays the jitCodeCache hashmap probe — that
            // probe was the source of the generic_dispatch_hot.mt regression
            // when added blindly to every IC hit.
            auto* cached = ctx->program->findCachedState(bytecodeOffset);

            if (cached && cached->jitProbeDone)
            {
                // Native callee: cached delegate, invoke directly.
                if (cached->cachedNativeFunc && ctx->vm)
                {
                    environment::NativeContext nativeCtx{ ctx->vm->getEnvironment(), ctx->vm->shared_from_this() };
                    ctx->returnValue = cached->cachedNativeFunc(
                        nativeCtx, std::span<const value::Value>(ctx->callArgs, argCount));
                    ctx->hasReturnValue = true;
                    return;
                }
                // Interpreter-only callee (most common: small bodies that
                // never crossed the JIT compilation threshold). Cached
                // FunctionMetadata lets us skip the program->getFunction
                // hashmap probe.
                if (cached->cachedFuncMetadata && ctx->vm)
                {
                    ctx->returnValue = ctx->vm->callFunctionFromJitDirect(
                        funcName, cached->cachedFuncMetadata,
                        std::span<const value::Value>(ctx->callArgs, argCount));
                    ctx->hasReturnValue = true;
                    return;
                }
                // jitProbeDone but no cached target — name didn't resolve last
                // time. Fall through to the throw at the end with a fresh lookup.
            }

            // Cold path: first call at this IP. Probe native + funcMeta and
            // populate the IC. Deliberately do NOT cache the JIT-compiled
            // entry here — calling JIT-compiled callees from inside the
            // OSR'd outer loop nests asmjit frames, and per MYT-184 that
            // path corrupted the native stack on some Math::-style callees
            // (silent /GS cookie failure / STATUS_STACK_BUFFER_OVERRUN).
            // Routing all IC hits through callFunctionFromJitDirect (mini-
            // interpreter) keeps both static_call_hot.mt and
            // generic_dispatch_hot.mt correct; speeding up the leaf-static
            // case is a follow-up that needs a JIT-side inliner for static
            // methods (mirrors the MYT-163 instance-method inliner).
            ::environment::registry::NativeDelegate nativeFn{};
            auto nativeRegistry = ctx->environment ? ctx->environment->getNativeRegistry() : nullptr;
            if (nativeRegistry && nativeRegistry->hasNativeFunction(funcName))
            {
                nativeFn = nativeRegistry->findNativeFunction(funcName);
            }

            const bytecode::BytecodeProgram::FunctionMetadata* funcMeta = nullptr;
            if (!nativeFn)
            {
                funcMeta = ctx->program->getFunction(funcName);
            }

            // Populate IC slot for warm path.
            {
                auto& slot = ctx->program->getOrCreateCachedState(bytecodeOffset);
                slot.cachedNativeFunc = nativeFn;
                if (funcMeta)
                {
                    slot.cachedFuncMetadata = funcMeta;
                    slot.cachedStartOffset = funcMeta->startOffset;
                    slot.cachedProgram = ctx->program;
                    slot.cachedProgramIndex = 0;
                }
                slot.jitProbeDone = true;
            }

            if (nativeFn && ctx->vm)
            {
                environment::NativeContext nativeCtx{ ctx->vm->getEnvironment(), ctx->vm->shared_from_this() };
                ctx->returnValue = nativeFn(
                    nativeCtx, std::span<const value::Value>(ctx->callArgs, argCount));
                ctx->hasReturnValue = true;
                return;
            }
            if (funcMeta && ctx->vm)
            {
                ctx->returnValue = ctx->vm->callFunctionFromJitDirect(
                    funcName, funcMeta,
                    std::span<const value::Value>(ctx->callArgs, argCount));
                ctx->hasReturnValue = true;
                return;
            }

            throw errors::RuntimeException("JIT: cannot call function '" + funcName + "'");
        }
        catch (...)
        {
            // MYT-268: includes the OSRDeoptException stashed by jit_await
            // (propagated via nestedCtx.pendingException; the prior
            // catch (OSRDeoptException&) wrapper here is unreachable).
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
                ctx->returnValue = ctx->vm->callFunctionFromJit(
                    funcName, std::span<const value::Value>(ctx->callArgs, argCount));
                ctx->hasReturnValue = true;
                return;
            }

            throw errors::RuntimeException("JIT: cannot call function '" + funcName + "'");
        }
        catch (...)
        {
            // MYT-268: includes the OSRDeoptException stashed by jit_await
            // (propagated via nestedCtx.pendingException; the prior
            // catch (OSRDeoptException&) wrapper here is unreachable).
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

