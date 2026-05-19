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

    bool emitFloatArithOps(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        if (instr.opcode != OpCode::ADD_FLOAT &&
            instr.opcode != OpCode::SUB_FLOAT &&
            instr.opcode != OpCode::MUL_FLOAT &&
            instr.opcode != OpCode::DIV_FLOAT)
            return false;

        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::FLOAT);
        emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::FLOAT);
        // MYT-211: reg-reg FLOAT op + publish.
        Vec right = consumeXmmHint(s, s.stackDepth);
        Vec left = consumeXmmHint(s, s.stackDepth - 1);
        if (instr.opcode == OpCode::ADD_FLOAT) cc.addsd(left, right);
        else if (instr.opcode == OpCode::SUB_FLOAT) cc.subsd(left, right);
        else if (instr.opcode == OpCode::MUL_FLOAT) cc.mulsd(left, right);
        else cc.divsd(left, right);
        s.slotTypes.push_back(SlotType::FLOAT);
        publishXmmHint(s, s.stackDepth - 1, left, /*dirty=*/false);
        return true;
    }
}
