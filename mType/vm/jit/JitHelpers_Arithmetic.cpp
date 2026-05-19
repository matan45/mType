#include "JitHelpers.hpp"
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <variant>
#include <vector>
#include "JitCodeCache.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/NativeContext.hpp"
#include "../../environment/registry/NativeRegistry.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../value/StringPool.hpp"
#include "../../value/ValueObject.hpp"
#include "../../value/ValueShim.hpp"

namespace vm::jit
{
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

        // Native recursion guard — fall back to interpreter (managed call stack)
        // to avoid overflowing the native C++ stack.
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

        if (nestedCtx.pendingException)
        {
            ctx->pendingException = nestedCtx.pendingException;
            return true;
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
        if (ctx->pendingException)
            return;

        ctx->hasReturnValue = false;
        ctx->returnValue = std::monostate{};

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
            // Function-level JIT bodies have no implicit pendingException check
            // between opcodes, so each helper's entry-check short-circuits until
            // the body returns and executeCallWithJit rethrows.
            // MYT-268: includes the OSRDeoptException stashed by jit_await
            // (now propagated via nestedCtx.pendingException in
            // tryJitDispatchResolved, then assigned to ctx->pendingException
            // before the helper returns).
            ctx->pendingException = std::current_exception();
        }
    }

    void jit_call_function_ic(JitContext* ctx, size_t bytecodeOffset,
                              uint32_t nameIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        ctx->hasReturnValue = false;
        ctx->returnValue = std::monostate{};

        try
        {
            const std::string& funcName = ctx->program->getConstantPool().getString(nameIndex);

            // IC slot lookup. The JIT-dispatch arm is split out into a "do we
            // even attempt tryJitDispatchResolved" gate (jitProbeDone) so a
            // callee that's NOT separately JIT-compiled never re-pays the
            // jitCodeCache hashmap probe — that probe was the source of the
            // generic_dispatch_hot.mt regression when added blindly to every IC hit.
            auto* cached = ctx->program->findCachedState(bytecodeOffset);

            if (cached && cached->jitProbeDone)
            {
                if (cached->cachedNativeFunc && ctx->vm)
                {
                    environment::NativeContext nativeCtx{ ctx->vm->getEnvironment(), ctx->vm->shared_from_this() };
                    ctx->returnValue = cached->cachedNativeFunc(
                        nativeCtx, std::span<const value::Value>(ctx->callArgs, argCount));
                    ctx->hasReturnValue = true;
                    return;
                }
                if (cached->cachedFuncMetadata && ctx->vm)
                {
                    // MYT-322: take the direct JIT-to-JIT path only when
                    // (a) the callee's instructionCount crosses the per-call-
                    // overhead break-even point, and (b) the callee has been
                    // JIT-compiled. Tiny leaves keep the cheaper mini-interpret
                    // fast path.
                    //
                    // Lazy refresh of cachedJitFnPtr mirrors JitHelpers_Objects
                    // pattern. Cold-fill at this IP runs before the callee
                    // tiers up; without the refresh, cachedJitFnPtr stays null
                    // and the direct path never engages even after function-
                    // entry tiering compiles the callee. Size threshold gating
                    // avoids the hashmap cost on tiny callees (MYT-321 revert
                    // cause).
                    void* directTarget = cached->cachedJitFnPtr;
                    if (cached->cachedFuncMetadata->instructionCount >= MIN_DIRECT_CALL_INSTRUCTION_COUNT)
                    {
                        if (!directTarget && ctx->jitCodeCache)
                        {
                            directTarget = reinterpret_cast<void*>(
                                ctx->jitCodeCache->lookup(funcName));
                            if (directTarget)
                            {
                                ctx->program->getOrCreateCachedState(bytecodeOffset)
                                    .cachedJitFnPtr = directTarget;
                            }
                        }
                        if (directTarget)
                        {
                            JitDirectCallArgs directArgs{
                                directTarget,
                                cached->cachedProgram,
                                cached->cachedFuncMetadata,
                                cached->cachedFrameName,
                                cached->cachedProgramIndex,
                                argCount};
                            jit_call_function_direct(ctx, directArgs);
                            return;
                        }
                    }
                    ctx->returnValue = ctx->vm->callFunctionFromJitDirect(
                        funcName, cached->cachedFuncMetadata,
                        std::span<const value::Value>(ctx->callArgs, argCount));
                    ctx->hasReturnValue = true;
                    return;
                }
                // jitProbeDone but no cached target — fall through to the
                // throw at the end with a fresh lookup.
            }

            // Cold path: first call at this IP. Probe native + funcMeta and
            // populate the IC. MYT-322 also probes JitCodeCache and pre-
            // interns the frame name + resolves the callee's programIndex so
            // the warm path's direct-dispatch branch can hand them straight
            // to jit_call_function_direct without re-doing the work.
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

            // MYT-322: resolve the callee's program index once. The old code
            // unconditionally stored `0`, which was wrong for library callees.
            size_t calleeProgramIndex = 0;
            if (funcMeta && ctx->vm)
            {
                const auto& loadedPrograms = ctx->vm->getLoadedPrograms();
                for (size_t i = 0; i < loadedPrograms.size(); ++i)
                {
                    if (loadedPrograms[i] == ctx->program) { calleeProgramIndex = i; break; }
                }
            }

            // MYT-322: probe JitCodeCache exactly once per IP. Native delegates
            // are never JIT-dispatched; skip the lookup when nativeFn is set.
            // The frame name is interned unconditionally when funcMeta resolved,
            // maintaining the invariant "frameName valid iff funcMeta cached".
            void* jitFnPtr = nullptr;
            bytecode::FunctionNameHandle frameName{ bytecode::INVALID_FN_HANDLE };
            if (funcMeta && !nativeFn)
            {
                if (ctx->jitCodeCache)
                {
                    jitFnPtr = reinterpret_cast<void*>(ctx->jitCodeCache->lookup(funcName));
                }
                frameName = ctx->program->internFrameName(funcName);
            }

            {
                auto& slot = ctx->program->getOrCreateCachedState(bytecodeOffset);
                slot.cachedNativeFunc = nativeFn;
                if (funcMeta)
                {
                    slot.cachedFuncMetadata = funcMeta;
                    slot.cachedStartOffset = funcMeta->startOffset;
                    slot.cachedProgram = ctx->program;
                    slot.cachedProgramIndex = calleeProgramIndex;
                    slot.cachedJitFnPtr = jitFnPtr;
                    slot.cachedFrameName = frameName;
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
            ctx->pendingException = std::current_exception();
        }
    }

    void jit_call_function_fast(JitContext* ctx, uint32_t funcIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        ctx->hasReturnValue = false;
        ctx->returnValue = std::monostate{};

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
            ctx->pendingException = std::current_exception();
        }
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
