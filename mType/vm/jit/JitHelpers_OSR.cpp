#include "JitHelpers.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../bytecode/OpCode.hpp"
#include "../../value/ValueShim.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
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

        // MYT-254: shared formatter for jit_trace_field_get / _set. Prints
        // the receiver's underlying ObjectInstance* (so we can tell whether
        // the receiver pointer drifts between iterations) and a 16-byte hex
        // dump of `payload` (the field-vector slot for GET; the new value
        // for SET).
        static void traceFieldImpl(const char* tag, uint64_t ip,
                                   uint64_t fieldIndex,
                                   const value::Value* receiver,
                                   const value::Value* payload)
        {
            std::cerr << tag
                      << " ip=" << ip
                      << " fieldIndex=" << fieldIndex
                      << " receiverPtr=0x" << std::hex
                      << reinterpret_cast<uintptr_t>(receiver) << std::dec;

            // Resolve receiver to the underlying ObjectInstance*. Guard
            // against null / unexpected tag so the probe never crashes the
            // process — we want trace lines, not segfaults.
            void* instancePtr = nullptr;
            int receiverTag = -1;
            if (receiver)
            {
                receiverTag = static_cast<int>(receiver->tag());
                if (value::isAnyObject(*receiver))
                {
                    instancePtr = static_cast<void*>(
                        value::asObjectInstanceRaw(*receiver));
                }
            }
            std::cerr << " receiverTag=" << receiverTag
                      << " instance=0x" << std::hex
                      << reinterpret_cast<uintptr_t>(instancePtr) << std::dec
                      << " payloadPtr=0x" << std::hex
                      << reinterpret_cast<uintptr_t>(payload) << std::dec;

            if (payload)
            {
                std::cerr << " payloadTag=" << static_cast<int>(payload->tag());
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(payload);
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
                std::cerr << " <null payload>";
            }
            std::cerr << "\n";
            std::cerr.flush();
        }

        void jit_trace_field_get(uint64_t ip, uint64_t fieldIndex,
                                 const value::Value* receiver,
                                 const value::Value* fieldData)
        {
            traceFieldImpl("[FIELD_GET]", ip, fieldIndex, receiver, fieldData);
        }

        void jit_trace_field_set(uint64_t ip, uint64_t fieldIndex,
                                 const value::Value* receiver,
                                 const value::Value* newValue)
        {
            traceFieldImpl("[FIELD_SET]", ip, fieldIndex, receiver, newValue);
        }

        // MYT-254: per-iteration receiver probe at the back-edge target
        // of the OSR'd loop. Prints `ip` (the back-edge target's bytecode
        // offset) and the address of locals[0] (the captured `this` for
        // method-bodied OSR) so we can verify the receiver pointer stays
        // stable across iterations of count()'s outer loop.
        void jit_trace_osr_loop_iter(uint64_t ip,
                                     const value::Value* receiver)
        {
            std::cerr << "[OSR-LOOP-ITER] ip=" << ip
                      << " receiverPtr=0x" << std::hex
                      << reinterpret_cast<uintptr_t>(receiver) << std::dec;
            void* instancePtr = nullptr;
            int receiverTag = -1;
            if (receiver)
            {
                receiverTag = static_cast<int>(receiver->tag());
                if (value::isAnyObject(*receiver))
                {
                    instancePtr = static_cast<void*>(
                        value::asObjectInstanceRaw(*receiver));
                }
            }
            std::cerr << " receiverTag=" << receiverTag
                      << " instance=0x" << std::hex
                      << reinterpret_cast<uintptr_t>(instancePtr) << std::dec
                      << "\n";
            std::cerr.flush();
        }

        // MYT-254 (E1/E2 disambiguation): slow-path helper-return probe.
        // Called immediately after a jit_get_field_ic or jit_call_method_ic
        // helper invoke — `returnValue` is the same memory location the
        // emitted code wrote into (boxedBase[..] for GET_FIELD,
        // ctx->returnValue for CALL_METHOD). helperKind is a stable
        // c-string baked at compile time.
        void jit_trace_helper_return(uint64_t ip, const char* helperKind,
                                     const value::Value* returnValue)
        {
            std::cerr << "[HELPER_RET] ip=" << ip
                      << " kind=" << (helperKind ? helperKind : "<null>")
                      << " ptr=0x" << std::hex
                      << reinterpret_cast<uintptr_t>(returnValue) << std::dec;
            if (returnValue)
            {
                std::cerr << " tag=" << static_cast<int>(returnValue->tag());
                const uint8_t* bytes =
                    reinterpret_cast<const uint8_t*>(returnValue);
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

        // MYT-254 (F-a/F-b disambiguation): print the receiver Value handed
        // to the slow-path CALL_METHOD helper. Resolves to the underlying
        // ObjectInstance* the same way traceFieldImpl does so the address we
        // print is comparable against [INTERP_FIELD_SET instance=...] lines.
        void jit_trace_call_method_args(uint64_t ip,
                                        const value::Value* receiver,
                                        uint64_t argCount)
        {
            std::cerr << "[CALL_ARGS] ip=" << ip
                      << " argCount=" << argCount
                      << " receiverPtr=0x" << std::hex
                      << reinterpret_cast<uintptr_t>(receiver) << std::dec;

            void* instancePtr = nullptr;
            int receiverTag = -1;
            if (receiver)
            {
                receiverTag = static_cast<int>(receiver->tag());
                if (value::isAnyObject(*receiver))
                {
                    instancePtr = static_cast<void*>(
                        value::asObjectInstanceRaw(*receiver));
                }
            }
            std::cerr << " receiverTag=" << receiverTag
                      << " instance=0x" << std::hex
                      << reinterpret_cast<uintptr_t>(instancePtr) << std::dec;

            if (receiver)
            {
                const uint8_t* bytes =
                    reinterpret_cast<const uint8_t*>(receiver);
                std::cerr << " bytes=";
                std::cerr << std::hex;
                for (size_t i = 0; i < 16; ++i)
                {
                    std::cerr << static_cast<int>(bytes[i]) << " ";
                }
                std::cerr << std::dec;
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
