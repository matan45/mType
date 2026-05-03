#include "JitHelpers.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../bytecode/OpCode.hpp"
#include "../../value/ValueShim.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"

namespace vm::jit
{
    extern "C" {

        // MYT-254: see declaration in JitHelpers.hpp. Single-instruction
        // body in practice — operator bool on std::exception_ptr is the
        // standard null-test and the only portable way to read the pointer
        // (its layout differs between MSVC, libstdc++, and libc++).
        int64_t jit_has_pending_exception(const JitContext* ctx)
        {
            return (ctx && ctx->pendingException) ? 1 : 0;
        }

        void jit_osr_write_local(JitContext* ctx, size_t slot, const value::Value* val)
        {
            if (ctx->osrOutputLocals && slot < ctx->osrLocalCount)
            {
                ctx->osrOutputLocals[slot] = *val;
            }
        }

        void jit_osr_exit(JitContext* ctx, uint64_t exitOffset)
        {
            ctx->osrExitOffset = static_cast<size_t>(exitOffset);
            ctx->osrExited = true;
        }

        // MYT-251: push/pop the inlined callee's owner-class for field
        // access validation. The JIT-emit path lowers these to a few
        // inline mov/inc/dec instructions; these C functions exist as
        // a fallback / declared-but-unused path so the externally-linked
        // symbols stay defined for any caller.
        void jit_push_inlined_class(JitContext* ctx, const char* name)
        {
            if (!ctx) return;
            if (ctx->inlinedCallingClassDepth < JitContext::MAX_INLINED_CLASS_DEPTH)
            {
                ctx->inlinedCallingClassNames[ctx->inlinedCallingClassDepth] = name;
                ++ctx->inlinedCallingClassDepth;
            }
        }

        void jit_pop_inlined_class(JitContext* ctx)
        {
            if (!ctx) return;
            if (ctx->inlinedCallingClassDepth > 0)
                --ctx->inlinedCallingClassDepth;
        }

    } // extern "C"

    // MYT-259: push the function's return value onto the interpreter's
    // operand stack so the resumed RETURN_VALUE bytecode (dispatched at
    // ctx->osrExitOffset by the interpreter) can pop it via the normal
    // handleReturnValue() path. Free functions (not extern "C") because
    // value::Value's copy constructor + StackManager::push aren't ABI-stable
    // C symbols; the JIT calls these via cc.invoke with Compiler-managed
    // argument marshaling, which doesn't require C linkage.
    void jit_osr_push_value(JitContext* ctx, const value::Value* val)
    {
        ctx->stackManager->push(*val);
    }

    void jit_osr_push_int(JitContext* ctx, int64_t val)
    {
        ctx->stackManager->push(value::Value(val));
    }

    void jit_osr_push_float(JitContext* ctx, double val)
    {
        ctx->stackManager->push(value::Value(val));
    }

    void jit_osr_push_bool(JitContext* ctx, int64_t val)
    {
        ctx->stackManager->push(value::Value(val != 0));
    }
}
