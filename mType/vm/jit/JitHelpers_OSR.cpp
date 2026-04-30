#include "JitHelpers.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../bytecode/OpCode.hpp"
#include <iostream>

namespace vm::jit
{
    extern "C" {

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

        // MYT-251: runtime trace called from JIT-emitted code at the start
        // of each inlined-body opcode when MTYPE_TRACE_JIT_RUNTIME=1.
        // Always-on at the call site (the env-var check at emit time
        // decides whether to emit the cc.invoke at all), so this body is
        // executed unconditionally once the cc.invoke fires.
        void jit_trace_runtime_op(uint64_t ip, uint64_t opcodeByte,
                                  uint64_t stackDepth, uint64_t inlineLocalsBase)
        {
            const auto op = static_cast<bytecode::OpCode>(
                static_cast<uint8_t>(opcodeByte));
            std::cerr << "[JIT-RT] ip=" << ip
                      << " op=" << bytecode::getOpCodeName(op)
                      << " stackDepth=" << stackDepth
                      << " inlineLocalsBase=" << inlineLocalsBase
                      << "\n";
            std::cerr.flush();
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

        // MYT-251: dump a Value's tag and raw bytes. Useful for confirming
        // that the receiver at boxedBase[N] is actually an
        // OBJECT_INSTANCE/VALUE_OBJECT vs. a VOID/garbage left by the
        // donation pattern. Read tag through index() so we don't depend
        // on the in-memory tag layout.
        void jit_trace_value(uint64_t label, const value::Value* val)
        {
            std::cerr << "[JIT-RT-VAL] label=" << label
                      << " ptr=0x" << std::hex
                      << reinterpret_cast<uintptr_t>(val) << std::dec;
            if (val)
            {
                std::cerr << " tag=" << static_cast<int>(val->tag());
                // Dump first 16 bytes (sizeof(Value) on x64) as hex.
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(val);
                std::cerr << " bytes=";
                std::cerr << std::hex;
                for (size_t i = 0; i < 16; ++i)
                {
                    std::cerr << static_cast<int>(bytes[i]) << " ";
                }
                std::cerr << std::dec;
            }
            else
            {
                std::cerr << " <null pointer>";
            }
            std::cerr << "\n";
            std::cerr.flush();
        }

    } // extern "C"

    void jit_osr_deoptimize(JitContext* ctx, uint64_t bytecodeOffset)
    {
        throw OSRDeoptException(static_cast<size_t>(bytecodeOffset));
    }
}
