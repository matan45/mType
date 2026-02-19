#include "JitCompiler.hpp"
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    static bool emitSimpleIntArithOps(JitEmissionState& s,
                                      const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
        emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);
        switch (instr.opcode)
        {
            case OpCode::ADD_INT:
            {
                Gp right = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.add(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
                break;
            }
            case OpCode::SUB_INT:
            {
                Gp right = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.sub(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
                break;
            }
            case OpCode::MUL_INT:
            {
                Gp left = cc.new_gp64();
                Gp right = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.imul(left, right);
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
                break;
            }
            default: return false;
        }
        s.slotTypes.push_back(SlotType::INT);
        return true;
    }

    static bool emitDivIntOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
        emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);
        Gp right = cc.new_gp64();
        cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
        cc.test(right, right);
        Label notZero = cc.new_label();
        cc.jnz(notZero);
        InvokeNode* invDZ;
        cc.invoke(Out(invDZ), reinterpret_cast<uint64_t>(jit_throw_div_by_zero),
                  FuncSignature::build<void>());
        cc.bind(notZero);
        Gp left = cc.new_gp64();
        cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        Gp raxReg = cc.new_gp64();
        Gp rdxReg = cc.new_gp64();
        cc.mov(raxReg, left);
        cc.cqo(rdxReg, raxReg);
        cc.idiv(rdxReg, raxReg, right);
        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), raxReg);
        s.slotTypes.push_back(SlotType::INT);
        return true;
    }

    static bool emitUnaryIntOps(JitEmissionState& s,
                                 const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        switch (instr.opcode)
        {
            case OpCode::NEG:
                cc.neg(Mem(s.stackBase, (s.stackDepth - 1) * 8));
                return true;
            case OpCode::INC:
            {
                Gp tmp = cc.new_gp64();
                cc.mov(tmp, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.inc(tmp);
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), tmp);
                return true;
            }
            case OpCode::DEC:
            {
                Gp tmp = cc.new_gp64();
                cc.mov(tmp, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.dec(tmp);
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), tmp);
                return true;
            }
            default: return false;
        }
    }

    static bool emitIntArithmeticOps(JitEmissionState& s,
                                     const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::ADD_INT: case OpCode::SUB_INT: case OpCode::MUL_INT:
                return emitSimpleIntArithOps(s, instr);
            case OpCode::DIV_INT:
                return emitDivIntOp(s);
            case OpCode::NEG: case OpCode::INC: case OpCode::DEC:
                return emitUnaryIntOps(s, instr);
            default: return false;
        }
    }

    static bool emitFloatArithOps(JitEmissionState& s,
                                   const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::FLOAT);
        emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::FLOAT);
        Vec right = cc.new_xmm();
        Vec left = cc.new_xmm();
        cc.movsd(right, Mem(s.stackBase, s.stackDepth * 8));
        cc.movsd(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        if (instr.opcode == OpCode::ADD_FLOAT) cc.addsd(left, right);
        else if (instr.opcode == OpCode::SUB_FLOAT) cc.subsd(left, right);
        else if (instr.opcode == OpCode::MUL_FLOAT) cc.mulsd(left, right);
        else cc.divsd(left, right);
        cc.movsd(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
        s.slotTypes.push_back(SlotType::FLOAT);
        return true;
    }

    static bool emitGenericAddSubMul(JitEmissionState& s,
                                      const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        SlotType rType = popType(s), lType = popType(s);
        s.stackDepth--;

        if (isBoxedSlotType(lType)) { emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT); lType = SlotType::INT; }
        if (isBoxedSlotType(rType)) { emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT); rType = SlotType::INT; }

        if (lType == SlotType::INT && rType == SlotType::INT)
        {
            if (instr.opcode == OpCode::MUL)
            {
                Gp left = cc.new_gp64();
                Gp right = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.imul(left, right);
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
            }
            else
            {
                Gp right = cc.new_gp64();
                cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
                if (instr.opcode == OpCode::ADD)
                    cc.add(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
                else
                    cc.sub(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
            }
            s.slotTypes.push_back(SlotType::INT);
        }
        else if (lType == SlotType::FLOAT && rType == SlotType::FLOAT)
        {
            Vec right = cc.new_xmm();
            Vec left = cc.new_xmm();
            cc.movsd(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.movsd(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            if (instr.opcode == OpCode::ADD) cc.addsd(left, right);
            else if (instr.opcode == OpCode::SUB) cc.subsd(left, right);
            else cc.mulsd(left, right);
            cc.movsd(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
            s.slotTypes.push_back(SlotType::FLOAT);
        }
        else
        {
            uint64_t fn;
            if (instr.opcode == OpCode::ADD) fn = (uint64_t)jit_generic_add;
            else if (instr.opcode == OpCode::SUB) fn = (uint64_t)jit_generic_sub;
            else fn = (uint64_t)jit_generic_mul;
            emitGenericBinop(s, fn, lType, rType);
        }
        return true;
    }

    static bool emitGenericDivMod(JitEmissionState& s,
                                   const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        SlotType rType = popType(s), lType = popType(s);
        s.stackDepth--;

        if (isBoxedSlotType(lType)) { emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT); lType = SlotType::INT; }
        if (isBoxedSlotType(rType)) { emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT); rType = SlotType::INT; }

        if (lType == SlotType::INT && rType == SlotType::INT)
        {
            Gp right = cc.new_gp64();
            cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.test(right, right);
            Label nz = cc.new_label();
            cc.jnz(nz);
            InvokeNode* dz;
            cc.invoke(Out(dz), (uint64_t)jit_throw_div_by_zero,
                      FuncSignature::build<void>());
            cc.bind(nz);
            Gp left = cc.new_gp64();
            cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            Gp rax = cc.new_gp64();
            Gp rdx = cc.new_gp64();
            cc.mov(rax, left);
            cc.cqo(rdx, rax);
            cc.idiv(rdx, rax, right);
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8),
                   instr.opcode == OpCode::DIV ? rax : rdx);
            s.slotTypes.push_back(SlotType::INT);
        }
        else if (lType == SlotType::FLOAT && rType == SlotType::FLOAT
                 && instr.opcode == OpCode::DIV)
        {
            Vec right = cc.new_xmm();
            Vec left = cc.new_xmm();
            cc.movsd(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.movsd(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            Vec zero = cc.new_xmm();
            cc.xorpd(zero, zero);
            cc.ucomisd(right, zero);
            Label nz = cc.new_label();
            cc.jne(nz);
            InvokeNode* dz;
            cc.invoke(Out(dz), (uint64_t)jit_throw_div_by_zero,
                      FuncSignature::build<void>());
            cc.bind(nz);
            cc.divsd(left, right);
            cc.movsd(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
            s.slotTypes.push_back(SlotType::FLOAT);
        }
        else
        {
            uint64_t fn = (instr.opcode == OpCode::DIV)
                ? (uint64_t)jit_generic_div : (uint64_t)jit_generic_mod;
            emitGenericBinop(s, fn, lType, rType);
        }
        return true;
    }

    static bool emitGenericArithmeticOps(JitEmissionState& s,
                                          const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::ADD_FLOAT: case OpCode::SUB_FLOAT:
            case OpCode::MUL_FLOAT: case OpCode::DIV_FLOAT:
                return emitFloatArithOps(s, instr);
            case OpCode::ADD: case OpCode::SUB: case OpCode::MUL:
                return emitGenericAddSubMul(s, instr);
            case OpCode::DIV: case OpCode::MOD:
                return emitGenericDivMod(s, instr);
            default: return false;
        }
    }

    static void emitBooleanBinop(JitEmissionState& s, bool isAnd)
    {
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

    static bool emitLogicalOps(JitEmissionState& s,
                                const bytecode::BytecodeProgram::Instruction& instr)
    {
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

    bool emitArithmeticOps(JitEmissionState& s,
                           const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::ADD_INT: case OpCode::SUB_INT:
            case OpCode::MUL_INT: case OpCode::DIV_INT:
            case OpCode::NEG: case OpCode::INC: case OpCode::DEC:
                return emitIntArithmeticOps(s, instr);

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

            default:
                return false;
        }
    }
}
