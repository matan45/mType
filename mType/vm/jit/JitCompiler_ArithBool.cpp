#include "JitCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    namespace
    {
        void emitBooleanBinop(JitEmissionState& s, bool isAnd)
        {
            // MYT-211: AND/OR boolean emitter isn't hint-aware.
            flushAllHints(s);
            auto& cc = s.cc;
            s.stackDepth--;
            SlotType rType = popType(s);
            SlotType lType = popType(s);
            emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
            emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);
            Gp right = cc.new_gp64();
            Gp left = cc.new_gp64();
            cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            Gp r1 = cc.new_gp64();
            Gp r2 = cc.new_gp64();
            cc.xor_(r1, r1);
            cc.test(left, left);
            cc.setne(r1.r8());
            cc.xor_(r2, r2);
            cc.test(right, right);
            cc.setne(r2.r8());
            if (isAnd) cc.and_(r1, r2);
            else cc.or_(r1, r2);
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), r1);
            s.slotTypes.push_back(SlotType::BOOL);
        }
    }

    bool emitLogicalOps(JitEmissionState& s,
                        const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-211: NOT/AND/OR aren't hint-aware.
        flushAllHints(s);
        auto& cc = s.cc;
        switch (instr.opcode)
        {
            case OpCode::NOT:
            {
                Gp val = cc.new_gp64();
                Gp result = cc.new_gp64();
                cc.mov(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.xor_(result, result);
                cc.test(val, val);
                cc.sete(result.r8());
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
                if (!s.slotTypes.empty()) s.slotTypes.back() = SlotType::BOOL;
                return true;
            }
            case OpCode::AND: emitBooleanBinop(s, true); return true;
            case OpCode::OR:  emitBooleanBinop(s, false); return true;
            default: return false;
        }
    }
}
