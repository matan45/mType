#include "JitCompiler.hpp"
#include "JitCompiler_ControlFlow.hpp"
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

    // Defined in JitCompiler_ArithInt.cpp.
    bool emitIntArithmeticOps(JitEmissionState& s,
                              const bytecode::BytecodeProgram::Instruction& instr);
    bool emitGenericArithmeticOps(JitEmissionState& s,
                                   const bytecode::BytecodeProgram::Instruction& instr);
    bool emitLogicalOps(JitEmissionState& s,
                        const bytecode::BytecodeProgram::Instruction& instr);

    // Defined in JitCompiler_ArithInvoke.cpp.
    bool emitInvokePrimitiveOps(JitEmissionState& s,
                                 const bytecode::BytecodeProgram::Instruction& instr);

    bool emitArithmeticOps(JitEmissionState& s,
                           const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::ADD_INT: case OpCode::SUB_INT:
            case OpCode::MUL_INT: case OpCode::DIV_INT:
            case OpCode::NEG: case OpCode::INC: case OpCode::DEC:
            case OpCode::BITWISE_AND_OP: case OpCode::BITWISE_OR_OP:
            case OpCode::BITWISE_XOR_OP: case OpCode::BITWISE_NOT_OP:
            case OpCode::BITWISE_AND_INT: case OpCode::BITWISE_OR_INT:
            case OpCode::BITWISE_XOR_INT: case OpCode::BITWISE_NOT_INT:
            case OpCode::LEFT_SHIFT_OP: case OpCode::RIGHT_SHIFT_OP:
            case OpCode::LEFT_SHIFT_INT: case OpCode::RIGHT_SHIFT_INT:
                return emitIntArithmeticOps(s, instr);

            case OpCode::LOAD_LOAD_ADD_INT:
            case OpCode::LOAD_LOAD_SUB_INT:
            case OpCode::LOAD_LOAD_MUL_INT:
            {
                // MYT-202: compile-time fused LOAD_LOCAL s1 + LOAD_LOCAL s2 +
                // {ADD,SUB,MUL}_INT. De-fuse at JIT time; the JIT machine-
                // code output is the same as the unfused sequence.
                bytecode::BytecodeProgram::Instruction load1(
                    OpCode::LOAD_LOCAL, instr.inlineOperands[0]);
                bytecode::BytecodeProgram::Instruction load2(
                    OpCode::LOAD_LOCAL, instr.inlineOperands[1]);
                if (!emitControlFlowOps(s, load1, nullptr) ||
                    !emitControlFlowOps(s, load2, nullptr))
                {
                    s.compileFailed = true;
                    return true;
                }
                OpCode arith =
                    (instr.opcode == OpCode::LOAD_LOAD_ADD_INT) ? OpCode::ADD_INT :
                    (instr.opcode == OpCode::LOAD_LOAD_SUB_INT) ? OpCode::SUB_INT :
                                                                   OpCode::MUL_INT;
                bytecode::BytecodeProgram::Instruction arithInstr(arith);
                return emitIntArithmeticOps(s, arithInstr);
            }

            case OpCode::ADD_INT_STORE_LOCAL:
            {
                // MYT-202: fused ADD_INT + STORE_LOCAL dst. Emit unfused
                // equivalent via the existing emitters.
                bytecode::BytecodeProgram::Instruction addInt(OpCode::ADD_INT);
                if (!emitIntArithmeticOps(s, addInt))
                {
                    s.compileFailed = true;
                    return true;
                }
                bytecode::BytecodeProgram::Instruction store(
                    OpCode::STORE_LOCAL, instr.inlineOperands[0]);
                return emitControlFlowOps(s, store, nullptr);
            }

            case OpCode::ADD_INT_CONST:
            {
                // MYT-198: de-fuse at JIT time — emit equivalent of PUSH_INT
                // (from fusedSlot) + ADD_INT. Implemented inline rather than
                // through emitSimpleIntArithOps since the right operand is an
                // immediate, not a stack slot.
                // MYT-201: fused state lives on the side table; bail on
                // compile failure if the entry is missing.
                auto& cc = s.cc;
                const auto* state = s.program.findCachedState(s.currentIP);
                if (!state)
                {
                    s.compileFailed = true;
                    return true;
                }
                size_t constIdx = static_cast<size_t>(state->fusedSlot);
                if (constIdx >= s.program.getConstantPool().integers.size())
                {
                    s.compileFailed = true;
                    return true;
                }
                int64_t literal = s.program.getConstantPool().getInteger(constIdx);

                // Unbox tos to INT if necessary (same pattern as
                // emitSimpleIntArithOps). The interpreter won't have fused
                // unless the site was stably int-typed, so this is the hot
                // path; BOXED inputs still get coerced rather than bailing.
                SlotType lType = popType(s);
                emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);

                // MYT-211: reg-reg add of an immediate via consume+publish.
                Gp left = consumeGpHint(s, s.stackDepth - 1);
                Gp right = cc.new_gp64();
                cc.mov(right, literal);
                cc.add(left, right);
                s.slotTypes.push_back(SlotType::INT);
                publishGpHint(s, s.stackDepth - 1, left);
                return true;
            }

            case OpCode::ADD_FLOAT: case OpCode::SUB_FLOAT:
            case OpCode::MUL_FLOAT: case OpCode::DIV_FLOAT:
            case OpCode::ADD: case OpCode::SUB: case OpCode::MUL:
            case OpCode::DIV: case OpCode::MOD:
                return emitGenericArithmeticOps(s, instr);

            case OpCode::EQ:  case OpCode::EQ_INT: emitCmp(s, CmpOp::EQ); return true;
            case OpCode::NE:  case OpCode::NE_INT: emitCmp(s, CmpOp::NE); return true;
            case OpCode::LT:  case OpCode::LT_INT: emitCmp(s, CmpOp::LT); return true;
            case OpCode::GT:  case OpCode::GT_INT: emitCmp(s, CmpOp::GT); return true;
            case OpCode::LE:                        emitCmp(s, CmpOp::LE); return true;
            case OpCode::GE:                        emitCmp(s, CmpOp::GE); return true;

            case OpCode::NOT: case OpCode::AND: case OpCode::OR:
                return emitLogicalOps(s, instr);

            case OpCode::INVOKE_INT_ADD: case OpCode::INVOKE_INT_SUB:
            case OpCode::INVOKE_INT_MUL: case OpCode::INVOKE_INT_DIV:
            case OpCode::INVOKE_INT_MOD:
            case OpCode::INVOKE_INT_NEG: case OpCode::INVOKE_INT_ABS:
            case OpCode::INVOKE_INT_EQUALS: case OpCode::INVOKE_INT_COMPARE:
            case OpCode::INVOKE_INT_GET_VALUE:
            case OpCode::INVOKE_INT_LESS_THAN: case OpCode::INVOKE_INT_LESS_EQUAL:
            case OpCode::INVOKE_INT_GREATER_THAN: case OpCode::INVOKE_INT_GREATER_EQUAL:
            case OpCode::INVOKE_FLOAT_ADD: case OpCode::INVOKE_FLOAT_SUB:
            case OpCode::INVOKE_FLOAT_MUL: case OpCode::INVOKE_FLOAT_DIV:
            case OpCode::INVOKE_FLOAT_NEG: case OpCode::INVOKE_FLOAT_ABS:
            case OpCode::INVOKE_FLOAT_EQUALS: case OpCode::INVOKE_FLOAT_COMPARE:
            case OpCode::INVOKE_FLOAT_GET_VALUE:
            case OpCode::INVOKE_BOOL_GET_VALUE:
            case OpCode::INVOKE_FLOAT_LESS_THAN: case OpCode::INVOKE_FLOAT_LESS_EQUAL:
            case OpCode::INVOKE_FLOAT_GREATER_THAN: case OpCode::INVOKE_FLOAT_GREATER_EQUAL:
            case OpCode::INVOKE_BOOL_AND: case OpCode::INVOKE_BOOL_OR:
            case OpCode::INVOKE_BOOL_XOR: case OpCode::INVOKE_BOOL_NOT:
            case OpCode::INVOKE_BOOL_EQUALS:
            case OpCode::INVOKE_STRING_LENGTH: case OpCode::INVOKE_STRING_IS_EMPTY:
            case OpCode::INVOKE_STRING_EQUALS: case OpCode::INVOKE_STRING_CONCAT:
                return emitInvokePrimitiveOps(s, instr);

            default:
                return false;
        }
    }
}
