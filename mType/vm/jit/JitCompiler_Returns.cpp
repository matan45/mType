#include "JitCompiler_ControlFlow.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include "JitHelpers.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;

    namespace
    {
        void emitReturnPrimitive(JitEmissionState& s, SlotType retType)
        {
            // MYT-211: return path reads stack memory; flush before emitting
            // the helper invokes so the return value reflects the latest
            // cached state.
            flushAllHints(s);
            auto& cc = s.cc;
            constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

            if (s.usesBoxedTypes && isBoxedSlotType(retType))
            {
                Gp retAddr = cc.new_gp64();
                cc.lea(retAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_boxed),
                          FuncSignature::build<void, JitContext*, const value::Value*>());
                inv->set_arg(0, s.ctxPtr);
                inv->set_arg(1, retAddr);
            }
            else if (retType == SlotType::FLOAT)
            {
                Vec retVal = cc.new_xmm();
                cc.movsd(retVal, Mem(s.stackBase, s.stackDepth * 8));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_float),
                          FuncSignature::build<void, JitContext*, double>());
                inv->set_arg(0, s.ctxPtr);
                inv->set_arg(1, retVal);
            }
            else if (retType == SlotType::BOOL)
            {
                Gp retVal = cc.new_gp64();
                cc.mov(retVal, Mem(s.stackBase, s.stackDepth * 8));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_bool),
                          FuncSignature::build<void, JitContext*, int64_t>());
                inv->set_arg(0, s.ctxPtr);
                inv->set_arg(1, retVal);
            }
            else
            {
                Gp retVal = cc.new_gp64();
                cc.mov(retVal, Mem(s.stackBase, s.stackDepth * 8));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_int),
                          FuncSignature::build<void, JitContext*, int64_t>());
                inv->set_arg(0, s.ctxPtr);
                inv->set_arg(1, retVal);
            }
        }

        // MYT-259: emit a cc.invoke that pushes the JIT operand-stack top
        // onto the interpreter's operand stack via the appropriate
        // jit_osr_push_* helper. Used by OSR-emitted RETURN_VALUE /
        // CREATE_PROMISE_RETURN_VALUE so the resumed RETURN_VALUE bytecode
        // can pop the value normally. Caller must pass the slotType BEFORE
        // popping s.slotTypes.
        void emitOsrPushReturnValueToInterpStack(JitEmissionState& s, SlotType retType)
        {
            auto& cc = s.cc;
            constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
            const int topSlot = s.stackDepth - 1;

            if (isBoxedSlotType(retType))
            {
                // Boxed Value already lives at boxedBase[topSlot]. Pass its
                // address; helper does *val copy into stackManager.
                Gp valAddr = cc.new_gp64();
                cc.lea(valAddr, Mem(s.boxedBase, static_cast<int32_t>(topSlot * valueSize)));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_osr_push_value),
                          FuncSignature::build<void, JitContext*, const value::Value*>());
                inv->set_arg(0, s.ctxPtr);
                inv->set_arg(1, valAddr);
            }
            else if (retType == SlotType::FLOAT)
            {
                Vec val = cc.new_xmm();
                cc.movsd(val, Mem(s.stackBase, topSlot * 8));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_osr_push_float),
                          FuncSignature::build<void, JitContext*, double>());
                inv->set_arg(0, s.ctxPtr);
                inv->set_arg(1, val);
            }
            else
            {
                // INT / BOOL — raw int64 in stackBase[topSlot]. Helper picks
                // the right box variant based on which jit_osr_push_* we call.
                Gp val = cc.new_gp64();
                cc.mov(val, Mem(s.stackBase, topSlot * 8));
                const uint64_t fn = (retType == SlotType::BOOL)
                    ? reinterpret_cast<uint64_t>(jit_osr_push_bool)
                    : reinterpret_cast<uint64_t>(jit_osr_push_int);
                InvokeNode* inv;
                cc.invoke(Out(inv), fn,
                          FuncSignature::build<void, JitContext*, int64_t>());
                inv->set_arg(0, s.ctxPtr);
                inv->set_arg(1, val);
            }
        }
    }

    void emitReturnValueOp(JitEmissionState& s, const ExitHandler& onExit)
    {
        if (onExit)
        {
            // MYT-185/186/187: snapshot the popped slot type so inline onExit
            // handlers can choose the correct box/unbox direction when
            // materializing the return value at endLabel.
            s.lastReturnSlotType = topType(s);

            // MYT-259: in OSR context, the onExit handler is osrExit which
            // resumes the interpreter at a bytecode offset. The previous
            // behaviour passed target=0 meaning "resume after the loop", which
            // silently dropped the function return — the interpreter then
            // fell through to whatever code followed the loop. Instead, push
            // the return value to the interpreter's operand stack and resume
            // AT this RETURN_VALUE opcode — the interpreter then runs
            // handleReturnValue() which pops, async-wraps if needed, and does
            // the function-return semantics correctly.
            if (s.isOSRCompilation && s.inlineStack.empty())
            {
                emitOsrPushReturnValueToInterpStack(s, s.lastReturnSlotType);
                s.stackDepth--;
                popType(s);
                onExit(s, s.currentIP);
                return;
            }

            s.stackDepth--;
            popType(s);
            onExit(s, 0);
            return;
        }

        s.stackDepth--;
        SlotType retType = popType(s);
        emitReturnPrimitive(s, retType);
        emitCleanup(s);
        s.cc.ret();
    }

    void emitCallReturnValue(JitEmissionState& s,
                              const std::string& returnType, bool isPrimReturn)
    {
        // MYT-211: return-value plumbing crosses helper invokes; flush.
        flushAllHints(s);
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        if (isPrimReturn)
        {
            SlotType retSlot = SlotType::INT;
            if (returnType == "float") retSlot = SlotType::FLOAT;
            else if (returnType == "bool") retSlot = SlotType::BOOL;

            Gp retAddr = cc.new_gp64();
            cc.lea(retAddr, Mem(s.ctxPtr, offsetof(JitContext, returnValue)));
            emitUnbox(s, retAddr, s.stackDepth, retSlot);
            s.slotTypes.push_back(retSlot);
        }
        else
        {
            Gp retAddr = cc.new_gp64();
            cc.lea(retAddr, Mem(s.ctxPtr, offsetof(JitContext, returnValue)));
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            InvokeNode* cpInv;
            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            cpInv->set_arg(0, destAddr);
            cpInv->set_arg(1, retAddr);
            s.slotTypes.push_back(SlotType::BOXED);
        }
        s.stackDepth++;
    }
}
