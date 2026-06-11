#include "JitCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
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
        bool emitSimpleIntArithOps(JitEmissionState& s,
                                    const bytecode::BytecodeProgram::Instruction& instr)
        {
            auto& cc = s.cc;
            if (instr.opcode != OpCode::ADD_INT &&
                instr.opcode != OpCode::SUB_INT &&
                instr.opcode != OpCode::MUL_INT)
                return false;

            s.stackDepth--;
            SlotType rType = popType(s);
            SlotType lType = popType(s);
            emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
            emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);
            // MYT-211: reg-reg arith + memory store via publishGpHint.
            Gp right = consumeGpHint(s, s.stackDepth);
            Gp left = consumeGpHint(s, s.stackDepth - 1);
            switch (instr.opcode)
            {
                case OpCode::ADD_INT: cc.add(left, right); break;
                case OpCode::SUB_INT: cc.sub(left, right); break;
                case OpCode::MUL_INT: cc.imul(left, right); break;
                default: break;
            }
            s.slotTypes.push_back(SlotType::INT);
            publishGpHint(s, s.stackDepth - 1, left);
            return true;
        }

        bool emitDivIntOp(JitEmissionState& s)
        {
            // MYT-211: reg-based DIV with publish at end.
            auto& cc = s.cc;
            s.stackDepth--;
            SlotType rType = popType(s);
            SlotType lType = popType(s);
            emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
            emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);
            Gp right = consumeGpHint(s, s.stackDepth);
            Gp left = consumeGpHint(s, s.stackDepth - 1);
            cc.test(right, right);
            Label notZero = cc.new_label();
            cc.jnz(notZero);
            InvokeNode* invDZ;
            cc.invoke(Out(invDZ), reinterpret_cast<uint64_t>(jit_throw_div_by_zero),
                      FuncSignature::build<void>());
            cc.bind(notZero);
            Label normalDiv = cc.new_label();
            Label divDone = cc.new_label();
            Gp minValue = cc.new_gp64();
            cc.mov(minValue, std::numeric_limits<int64_t>::min());
            cc.cmp(right, -1);
            cc.jne(normalDiv);
            cc.cmp(left, minValue);
            cc.jne(normalDiv);
            cc.mov(left, minValue);
            cc.jmp(divDone);
            cc.bind(normalDiv);
            Gp raxReg = cc.new_gp64();
            Gp rdxReg = cc.new_gp64();
            cc.mov(raxReg, left);
            cc.cqo(rdxReg, raxReg);
            cc.idiv(rdxReg, raxReg, right);
            cc.mov(left, raxReg);
            cc.bind(divDone);
            s.slotTypes.push_back(SlotType::INT);
            publishGpHint(s, s.stackDepth - 1, left);
            return true;
        }

        bool emitUnaryIntOps(JitEmissionState& s,
                              const bytecode::BytecodeProgram::Instruction& instr)
        {
            // MYT-211: in-place unary on TOS — consume into reg, op, publish.
            auto& cc = s.cc;
            SlotType vType = popType(s);
            emitEnsureUnboxed(s, s.stackDepth - 1, vType, SlotType::INT);
            Gp tmp = consumeGpHint(s, s.stackDepth - 1);
            switch (instr.opcode)
            {
                case OpCode::NEG:                                           cc.neg(tmp); break;
                case OpCode::INC:                                           cc.inc(tmp); break;
                case OpCode::DEC:                                           cc.dec(tmp); break;
                case OpCode::BITWISE_NOT_OP: case OpCode::BITWISE_NOT_INT:  cc.not_(tmp); break;
                default: return false;
            }
            s.slotTypes.push_back(SlotType::INT);
            publishGpHint(s, s.stackDepth - 1, tmp);
            return true;
        }

        bool emitBitwiseIntOps(JitEmissionState& s,
                                const bytecode::BytecodeProgram::Instruction& instr)
        {
            auto& cc = s.cc;
            // Accept both the generic _OP form and the INT-specialized form;
            // the INT variant carries the same semantics — compiler/runtime
            // has just proven the operand types are INT at the bytecode level.
            OpCode opcode = instr.opcode;
            if (opcode == OpCode::BITWISE_AND_INT) opcode = OpCode::BITWISE_AND_OP;
            else if (opcode == OpCode::BITWISE_OR_INT)  opcode = OpCode::BITWISE_OR_OP;
            else if (opcode == OpCode::BITWISE_XOR_INT) opcode = OpCode::BITWISE_XOR_OP;
            if (opcode != OpCode::BITWISE_AND_OP &&
                opcode != OpCode::BITWISE_OR_OP &&
                opcode != OpCode::BITWISE_XOR_OP)
                return false;

            s.stackDepth--;
            SlotType rType = popType(s);
            SlotType lType = popType(s);
            emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
            emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);

            Gp right = cc.new_gp64();
            Gp left = cc.new_gp64();
            cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            switch (opcode)
            {
                case OpCode::BITWISE_AND_OP: cc.and_(left, right); break;
                case OpCode::BITWISE_OR_OP:  cc.or_(left, right); break;
                case OpCode::BITWISE_XOR_OP: cc.xor_(left, right); break;
                default: break;
            }
            s.slotTypes.push_back(SlotType::INT);
            publishGpHint(s, s.stackDepth - 1, left);
            return true;
        }

        bool emitShiftOp(JitEmissionState& s,
                          const bytecode::BytecodeProgram::Instruction& instr)
        {
            auto& cc = s.cc;
            OpCode opcode = instr.opcode;
            if (opcode == OpCode::LEFT_SHIFT_INT)  opcode = OpCode::LEFT_SHIFT_OP;
            else if (opcode == OpCode::RIGHT_SHIFT_INT) opcode = OpCode::RIGHT_SHIFT_OP;
            if (opcode != OpCode::LEFT_SHIFT_OP &&
                opcode != OpCode::RIGHT_SHIFT_OP)
                return false;

            s.stackDepth--;
            SlotType rType = popType(s);
            SlotType lType = popType(s);
            emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
            emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);

            // MYT-211: peephole — when the right operand is a known constant
            // in [0, 63] (as recorded by publishConstHint from PUSH_INT),
            // skip the runtime range check and emit the shift with an
            // immediate operand.
            bool constFold = false;
            int64_t shamt = 0;
            if (static_cast<size_t>(s.stackDepth) < s.slotHints.size())
            {
                SlotHint& rh = s.slotHints[s.stackDepth];
                if (rh.valid && rh.isConstant && rh.constValue >= 0 && rh.constValue <= 63)
                {
                    constFold = true;
                    shamt = rh.constValue;
                    rh.valid = false; rh.isConstant = false;
                }
            }

            if (constFold)
            {
                Gp left = consumeGpHint(s, s.stackDepth - 1);
                if (opcode == OpCode::LEFT_SHIFT_OP)
                    cc.sal(left, Imm(static_cast<uint32_t>(shamt)));
                else
                    cc.sar(left, Imm(static_cast<uint32_t>(shamt)));
                s.slotTypes.push_back(SlotType::INT);
                publishGpHint(s, s.stackDepth - 1, left);
                return true;
            }

            Gp count = consumeGpHint(s, s.stackDepth);
            Gp left = consumeGpHint(s, s.stackDepth - 1);

            Label inRange = cc.new_label();
            cc.cmp(count, 63);
            cc.jbe(inRange);
            InvokeNode* invOob;
            cc.invoke(Out(invOob), reinterpret_cast<uint64_t>(jit_throw_shift_out_of_range),
                      FuncSignature::build<void, int64_t>());
            invOob->set_arg(0, count);
            cc.bind(inRange);

            if (opcode == OpCode::LEFT_SHIFT_OP)
                cc.sal(left, count.r8());
            else
                cc.sar(left, count.r8());

            s.slotTypes.push_back(SlotType::INT);
            publishGpHint(s, s.stackDepth - 1, left);
            return true;
        }
    }

    bool emitIntArithmeticOps(JitEmissionState& s,
                              const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::ADD_INT: case OpCode::SUB_INT: case OpCode::MUL_INT:
                return emitSimpleIntArithOps(s, instr);
            case OpCode::DIV_INT:
                return emitDivIntOp(s);
            case OpCode::NEG: case OpCode::INC: case OpCode::DEC:
            case OpCode::BITWISE_NOT_OP:
            case OpCode::BITWISE_NOT_INT:
                return emitUnaryIntOps(s, instr);
            case OpCode::BITWISE_AND_OP: case OpCode::BITWISE_OR_OP: case OpCode::BITWISE_XOR_OP:
            case OpCode::BITWISE_AND_INT: case OpCode::BITWISE_OR_INT: case OpCode::BITWISE_XOR_INT:
                return emitBitwiseIntOps(s, instr);
            case OpCode::LEFT_SHIFT_OP: case OpCode::RIGHT_SHIFT_OP:
            case OpCode::LEFT_SHIFT_INT: case OpCode::RIGHT_SHIFT_INT:
                return emitShiftOp(s, instr);
            default: return false;
        }
    }
}
