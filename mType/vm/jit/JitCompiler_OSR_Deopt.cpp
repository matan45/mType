#include "JitCompiler_OSR_Internal.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    static void emitWriteBackNonBoxedLocal(Compiler& cc, Gp destAddr,
                                             Gp localsBase, size_t slot, SlotType lt)
    {
        if (lt == SlotType::FLOAT)
        {
            Vec val = cc.new_xmm();
            cc.movsd(val, Mem(localsBase, static_cast<int32_t>(slot * 8)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_float),
                      FuncSignature::build<void, value::Value*, double>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, val);
        }
        else
        {
            Gp val = cc.new_gp64();
            cc.mov(val, Mem(localsBase, static_cast<int32_t>(slot * 8)));
            uint64_t fn = (lt == SlotType::BOOL)
                ? reinterpret_cast<uint64_t>(jit_box_bool)
                : reinterpret_cast<uint64_t>(jit_box_int);
            InvokeNode* inv;
            cc.invoke(Out(inv), fn,
                      FuncSignature::build<void, value::Value*, int64_t>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, val);
        }
    }

    void emitLocalsWriteBack(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        Gp outPtr = cc.new_gp64();
        cc.mov(outPtr, Mem(s.ctxPtr, offsetof(JitContext, osrOutputLocals)));

        for (size_t i = 0; i < s.localCount; ++i)
        {
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(outPtr, static_cast<int32_t>(i * valueSize)));
            SlotType lt = s.localTypes.count(i) ? s.localTypes[i] : SlotType::INT;

            if (s.usesBoxedTypes)
            {
                Gp srcAddr = cc.new_gp64();
                cc.lea(srcAddr, Mem(s.localsBase, static_cast<int32_t>(i * s.localStride)));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                          FuncSignature::build<void, value::Value*, const value::Value*>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, srcAddr);
            }
            else
            {
                emitWriteBackNonBoxedLocal(cc, destAddr, s.localsBase, i, lt);
            }
        }
    }

    // MYT-211: scan the OSR loop range for slots that are read but never
    // written inside the loop. Hoist a single load of each such slot into a
    // virtreg at OSR-prologue end so emitLoadLocal can short-circuit to the
    // cached register on every iteration. This eliminates the per-iter memory
    // load for loop-invariant locals.
    //
    // Currently dead code — the call site in emitOSRBody is commented out
    // pending a fix to virtreg liveness across the prologue→loop-body cc.jmp.
    // Kept for re-enable after debugging confirms safe.
    void hoistInvariantLocals(JitEmissionState& s,
                              const bytecode::BytecodeProgram& program,
                              size_t loopStartOffset, size_t loopEndOffset)
    {
        if (s.usesBoxedTypes) return;
        std::unordered_set<size_t> writtenSlots;
        std::unordered_set<size_t> readSlots;
        for (size_t ip = loopStartOffset; ip <= loopEndOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            switch (instr.opcode)
            {
                case OpCode::STORE_LOCAL:
                case OpCode::STORE_LOCAL_INT:
                case OpCode::STORE_LOCAL_FLOAT:
                case OpCode::STORE_LOCAL_BOOL:
                case OpCode::STORE_LOCAL_BOXED_INST:
                case OpCode::ADD_INT_STORE_LOCAL:        // dst slot in operand[0]
                    if (instr.hasOperands())
                        writtenSlots.insert(instr.inlineOperands[0]);
                    break;
                case OpCode::LOAD_STORE_LOCAL:           // src=op[0], dst=op[1]
                    if (instr.numOperands() >= 2)
                    {
                        readSlots.insert(instr.inlineOperands[0]);
                        writtenSlots.insert(instr.inlineOperands[1]);
                    }
                    break;
                case OpCode::LOAD_LOCAL:
                case OpCode::LOAD_LOCAL_INT:
                case OpCode::LOAD_LOCAL_FLOAT:
                case OpCode::LOAD_LOCAL_BOOL:
                case OpCode::LOAD_LOCAL_BOXED_INST:
                    if (instr.hasOperands())
                        readSlots.insert(instr.inlineOperands[0]);
                    break;
                case OpCode::LOAD_LOAD_ADD_INT:
                case OpCode::LOAD_LOAD_SUB_INT:
                case OpCode::LOAD_LOAD_MUL_INT:
                    if (instr.numOperands() >= 2)
                    {
                        readSlots.insert(instr.inlineOperands[0]);
                        readSlots.insert(instr.inlineOperands[1]);
                    }
                    break;
                default: break;
            }
        }
        auto& cc = s.cc;
        for (size_t slot : readSlots)
        {
            if (writtenSlots.count(slot)) continue;
            SlotType lt = s.localTypes.count(slot) ? s.localTypes[slot] : SlotType::INT;
            if (isBoxedSlotType(lt)) continue;       // boxed locals stay memory-only
            if (lt == SlotType::FLOAT)
            {
                Vec reg = cc.new_xmm();
                cc.movsd(reg, Mem(s.localsBase, static_cast<int32_t>(slot * 8)));
                s.invariantFloatLocals[slot] = reg;
            }
            else
            {
                Gp reg = cc.new_gp64();
                cc.mov(reg, Mem(s.localsBase, static_cast<int32_t>(slot * 8)));
                s.invariantIntLocals[slot] = reg;
            }
        }
    }
}
