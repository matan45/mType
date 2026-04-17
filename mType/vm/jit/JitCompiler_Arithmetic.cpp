#include "JitCompiler.hpp"
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "ic/TypeFeedbackCollector.hpp"
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
        // Validate opcode before mutating state
        if (instr.opcode != OpCode::ADD_INT &&
            instr.opcode != OpCode::SUB_INT &&
            instr.opcode != OpCode::MUL_INT)
            return false;

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
            default: break;
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
            case OpCode::BITWISE_NOT_OP:
                cc.not_(Mem(s.stackBase, (s.stackDepth - 1) * 8));
                return true;
            default: return false;
        }
    }

    static bool emitBitwiseIntOps(JitEmissionState& s,
                                   const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        if (instr.opcode != OpCode::BITWISE_AND_OP &&
            instr.opcode != OpCode::BITWISE_OR_OP &&
            instr.opcode != OpCode::BITWISE_XOR_OP)
            return false;

        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
        emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);

        Gp right = cc.new_gp64();
        cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
        switch (instr.opcode)
        {
            case OpCode::BITWISE_AND_OP:
                cc.and_(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
                break;
            case OpCode::BITWISE_OR_OP:
                cc.or_(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
                break;
            case OpCode::BITWISE_XOR_OP:
                cc.xor_(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
                break;
            default: break;
        }
        s.slotTypes.push_back(SlotType::INT);
        return true;
    }

    static bool emitShiftOp(JitEmissionState& s,
                             const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        if (instr.opcode != OpCode::LEFT_SHIFT_OP &&
            instr.opcode != OpCode::RIGHT_SHIFT_OP)
            return false;

        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
        emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);

        Gp count = cc.new_gp64();
        cc.mov(count, Mem(s.stackBase, s.stackDepth * 8));

        // Unsigned compare catches both negative (huge unsigned) and > 63
        Label inRange = cc.new_label();
        cc.cmp(count, 63);
        cc.jbe(inRange);
        InvokeNode* invOob;
        cc.invoke(Out(invOob), reinterpret_cast<uint64_t>(jit_throw_shift_out_of_range),
                  FuncSignature::build<void, int64_t>());
        invOob->set_arg(0, count);
        cc.bind(inRange);

        if (instr.opcode == OpCode::LEFT_SHIFT_OP)
            cc.sal(Mem(s.stackBase, (s.stackDepth - 1) * 8), count.r8());
        else
            cc.sar(Mem(s.stackBase, (s.stackDepth - 1) * 8), count.r8());

        s.slotTypes.push_back(SlotType::INT);
        return true;
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
            case OpCode::BITWISE_NOT_OP:
                return emitUnaryIntOps(s, instr);
            case OpCode::BITWISE_AND_OP: case OpCode::BITWISE_OR_OP: case OpCode::BITWISE_XOR_OP:
                return emitBitwiseIntOps(s, instr);
            case OpCode::LEFT_SHIFT_OP: case OpCode::RIGHT_SHIFT_OP:
                return emitShiftOp(s, instr);
            default: return false;
        }
    }

    static bool emitFloatArithOps(JitEmissionState& s,
                                   const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        // Validate opcode before mutating state
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

        // Determine unbox target: if either operand is FLOAT, unbox as FLOAT
        SlotType unboxTarget = (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
            ? SlotType::FLOAT : SlotType::INT;
        // Use type feedback to specialize when both operands are boxed
        if (unboxTarget == SlotType::INT && isBoxedSlotType(lType) && isBoxedSlotType(rType)
            && s.typeFeedback && s.typeFeedback->shouldSpecialize(s.currentIP))
        {
            auto [lt, rt] = s.typeFeedback->getDominantTypes(s.currentIP);
            if (lt == ic::ObservedType::FLOAT || rt == ic::ObservedType::FLOAT)
                unboxTarget = SlotType::FLOAT;
        }
        if (isBoxedSlotType(lType)) { emitEnsureUnboxed(s, s.stackDepth - 1, lType, unboxTarget); lType = unboxTarget; }
        if (isBoxedSlotType(rType)) { emitEnsureUnboxed(s, s.stackDepth, rType, unboxTarget); rType = unboxTarget; }

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

        // Determine unbox target: if either operand is FLOAT, unbox as FLOAT
        SlotType unboxTarget = (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
            ? SlotType::FLOAT : SlotType::INT;
        // Use type feedback to specialize when both operands are boxed
        if (unboxTarget == SlotType::INT && isBoxedSlotType(lType) && isBoxedSlotType(rType)
            && s.typeFeedback && s.typeFeedback->shouldSpecialize(s.currentIP))
        {
            auto [lt, rt] = s.typeFeedback->getDominantTypes(s.currentIP);
            if (lt == ic::ObservedType::FLOAT || rt == ic::ObservedType::FLOAT)
                unboxTarget = SlotType::FLOAT;
        }
        if (isBoxedSlotType(lType)) { emitEnsureUnboxed(s, s.stackDepth - 1, lType, unboxTarget); lType = unboxTarget; }
        if (isBoxedSlotType(rType)) { emitEnsureUnboxed(s, s.stackDepth, rType, unboxTarget); rType = unboxTarget; }

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
            cc.jp(nz);   // Jump if NaN (parity flag set by ucomisd on unordered comparison)
            cc.jne(nz);  // Jump if non-zero
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

    // MYT-144: specialized primitive method opcodes (INVOKE_INT_* / INVOKE_FLOAT_*).
    // The bytecode compiler emits these when it can statically prove a method call
    // reduces to a known scalar op on a boxed Int / Float receiver. Inputs arrive
    // on the operand stack as boxed primitives (or already-unboxed raw primitives
    // from a prior INVOKE_*); emitEnsureUnboxed normalizes both cases to raw
    // int64 / double. Result is pushed as a raw primitive (lazy-rebox) to match
    // the interpreter contract in PrimitiveMethodExecutor.
    static bool emitInvokeIntBinary(JitEmissionState& s, OpCode op)
    {
        auto& cc = s.cc;
        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
        emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);

        if (op == OpCode::INVOKE_INT_DIV || op == OpCode::INVOKE_INT_MOD)
        {
            Gp right = cc.new_gp64();
            cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.test(right, right);
            Label nz = cc.new_label();
            cc.jnz(nz);
            InvokeNode* dz;
            cc.invoke(Out(dz), reinterpret_cast<uint64_t>(jit_throw_div_by_zero),
                      FuncSignature::build<void>());
            cc.bind(nz);
            Gp left = cc.new_gp64();
            cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            Gp raxReg = cc.new_gp64();
            Gp rdxReg = cc.new_gp64();
            cc.mov(raxReg, left);
            cc.cqo(rdxReg, raxReg);
            cc.idiv(rdxReg, raxReg, right);
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8),
                   op == OpCode::INVOKE_INT_DIV ? raxReg : rdxReg);
            s.slotTypes.push_back(SlotType::INT);
            return true;
        }

        if (op == OpCode::INVOKE_INT_ADD || op == OpCode::INVOKE_INT_SUB)
        {
            Gp right = cc.new_gp64();
            cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
            if (op == OpCode::INVOKE_INT_ADD)
                cc.add(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
            else
                cc.sub(Mem(s.stackBase, (s.stackDepth - 1) * 8), right);
            s.slotTypes.push_back(SlotType::INT);
            return true;
        }

        if (op == OpCode::INVOKE_INT_MUL)
        {
            Gp left = cc.new_gp64();
            Gp right = cc.new_gp64();
            cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.imul(left, right);
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
            s.slotTypes.push_back(SlotType::INT);
            return true;
        }

        if (op == OpCode::INVOKE_INT_EQUALS)
        {
            Gp left = cc.new_gp64();
            Gp right = cc.new_gp64();
            cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
            Gp result = cc.new_gp64();
            cc.xor_(result, result);
            cc.cmp(left, right);
            cc.sete(result.r8());
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
            s.slotTypes.push_back(SlotType::BOOL);
            return true;
        }

        if (op == OpCode::INVOKE_INT_COMPARE)
        {
            // Three-way: (l < r) ? -1 : (l > r) ? 1 : 0
            Gp left = cc.new_gp64();
            Gp right = cc.new_gp64();
            cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
            Gp gt = cc.new_gp64();
            Gp lt = cc.new_gp64();
            cc.xor_(gt, gt);
            cc.xor_(lt, lt);
            cc.cmp(left, right);
            cc.setg(gt.r8());
            cc.setl(lt.r8());
            cc.sub(gt, lt);
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), gt);
            s.slotTypes.push_back(SlotType::INT);
            return true;
        }

        return false;
    }

    static bool emitInvokeIntUnary(JitEmissionState& s, OpCode op)
    {
        auto& cc = s.cc;
        SlotType rType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth - 1, rType, SlotType::INT);

        if (op == OpCode::INVOKE_INT_NEG)
        {
            cc.neg(Mem(s.stackBase, (s.stackDepth - 1) * 8));
        }
        else // INVOKE_INT_ABS: abs(x) via sign-mask trick.
        {
            Gp val = cc.new_gp64();
            Gp mask = cc.new_gp64();
            cc.mov(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.mov(mask, val);
            cc.sar(mask, 63);        // mask = sign-extended (0 or -1)
            cc.xor_(val, mask);
            cc.sub(val, mask);
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), val);
        }
        s.slotTypes.push_back(SlotType::INT);
        return true;
    }

    static bool emitInvokeFloatBinary(JitEmissionState& s, OpCode op)
    {
        auto& cc = s.cc;
        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::FLOAT);
        emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::FLOAT);

        if (op == OpCode::INVOKE_FLOAT_DIV)
        {
            Vec left = cc.new_xmm();
            Vec right = cc.new_xmm();
            cc.movsd(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.movsd(right, Mem(s.stackBase, s.stackDepth * 8));
            Vec zero = cc.new_xmm();
            cc.xorpd(zero, zero);
            cc.ucomisd(right, zero);
            Label nz = cc.new_label();
            cc.jp(nz);   // unordered (NaN): skip zero trap
            cc.jne(nz);  // non-zero: skip zero trap
            InvokeNode* dz;
            cc.invoke(Out(dz), reinterpret_cast<uint64_t>(jit_throw_div_by_zero),
                      FuncSignature::build<void>());
            cc.bind(nz);
            cc.divsd(left, right);
            cc.movsd(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
            s.slotTypes.push_back(SlotType::FLOAT);
            return true;
        }

        if (op == OpCode::INVOKE_FLOAT_ADD || op == OpCode::INVOKE_FLOAT_SUB
            || op == OpCode::INVOKE_FLOAT_MUL)
        {
            Vec left = cc.new_xmm();
            Vec right = cc.new_xmm();
            cc.movsd(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.movsd(right, Mem(s.stackBase, s.stackDepth * 8));
            if (op == OpCode::INVOKE_FLOAT_ADD) cc.addsd(left, right);
            else if (op == OpCode::INVOKE_FLOAT_SUB) cc.subsd(left, right);
            else cc.mulsd(left, right);
            cc.movsd(Mem(s.stackBase, (s.stackDepth - 1) * 8), left);
            s.slotTypes.push_back(SlotType::FLOAT);
            return true;
        }

        if (op == OpCode::INVOKE_FLOAT_EQUALS)
        {
            // NaN equality matches interpreter: ucomisd sets ZF=0 for unordered,
            // so sete naturally gives 0 on NaN (matching == semantics).
            Vec left = cc.new_xmm();
            Vec right = cc.new_xmm();
            cc.movsd(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.movsd(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.ucomisd(left, right);
            Gp eq = cc.new_gp64();
            Gp ordered = cc.new_gp64();
            cc.xor_(eq, eq);
            cc.xor_(ordered, ordered);
            cc.sete(eq.r8());
            cc.setnp(ordered.r8());
            cc.and_(eq, ordered);   // NaN: PF=1 -> ordered=0 -> eq masked to 0
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), eq);
            s.slotTypes.push_back(SlotType::BOOL);
            return true;
        }

        if (op == OpCode::INVOKE_FLOAT_COMPARE)
        {
            // Three-way w/ NaN -> 0 (interpreter semantics: both < and > false on NaN).
            Vec left = cc.new_xmm();
            Vec right = cc.new_xmm();
            cc.movsd(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.movsd(right, Mem(s.stackBase, s.stackDepth * 8));
            cc.ucomisd(left, right);
            Gp gt = cc.new_gp64();
            Gp ltRaw = cc.new_gp64();
            Gp ordered = cc.new_gp64();
            cc.xor_(gt, gt);
            cc.xor_(ltRaw, ltRaw);
            cc.xor_(ordered, ordered);
            cc.seta(gt.r8());       // CF=0 && ZF=0 -> l>r (false on NaN since CF=1)
            cc.setb(ltRaw.r8());    // CF=1 -> l<r OR unordered
            cc.setnp(ordered.r8()); // 1 iff ordered
            cc.and_(ltRaw, ordered);// mask: only ordered l<r
            cc.sub(gt, ltRaw);
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), gt);
            s.slotTypes.push_back(SlotType::INT);
            return true;
        }

        return false;
    }

    static bool emitInvokeFloatUnary(JitEmissionState& s, OpCode op)
    {
        auto& cc = s.cc;
        SlotType rType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth - 1, rType, SlotType::FLOAT);

        Vec val = cc.new_xmm();
        cc.movsd(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));

        if (op == OpCode::INVOKE_FLOAT_NEG)
        {
            // Flip sign bit via XOR with 0x8000000000000000.
            Gp mask = cc.new_gp64();
            cc.mov(mask, static_cast<int64_t>(0x8000000000000000ULL));
            Vec maskVec = cc.new_xmm();
            cc.movq(maskVec, mask);
            cc.xorpd(val, maskVec);
        }
        else // INVOKE_FLOAT_ABS: clear sign bit via AND with 0x7FFFFFFFFFFFFFFF.
        {
            Gp mask = cc.new_gp64();
            cc.mov(mask, static_cast<int64_t>(0x7FFFFFFFFFFFFFFFLL));
            Vec maskVec = cc.new_xmm();
            cc.movq(maskVec, mask);
            cc.andpd(val, maskVec);
        }

        cc.movsd(Mem(s.stackBase, (s.stackDepth - 1) * 8), val);
        s.slotTypes.push_back(SlotType::FLOAT);
        return true;
    }

    static bool emitInvokePrimitiveOps(JitEmissionState& s,
                                        const bytecode::BytecodeProgram::Instruction& instr)
    {
        switch (instr.opcode)
        {
            case OpCode::INVOKE_INT_ADD: case OpCode::INVOKE_INT_SUB:
            case OpCode::INVOKE_INT_MUL: case OpCode::INVOKE_INT_DIV:
            case OpCode::INVOKE_INT_MOD:
            case OpCode::INVOKE_INT_EQUALS: case OpCode::INVOKE_INT_COMPARE:
                return emitInvokeIntBinary(s, instr.opcode);

            case OpCode::INVOKE_INT_NEG: case OpCode::INVOKE_INT_ABS:
                return emitInvokeIntUnary(s, instr.opcode);

            case OpCode::INVOKE_FLOAT_ADD: case OpCode::INVOKE_FLOAT_SUB:
            case OpCode::INVOKE_FLOAT_MUL: case OpCode::INVOKE_FLOAT_DIV:
            case OpCode::INVOKE_FLOAT_EQUALS: case OpCode::INVOKE_FLOAT_COMPARE:
                return emitInvokeFloatBinary(s, instr.opcode);

            case OpCode::INVOKE_FLOAT_NEG: case OpCode::INVOKE_FLOAT_ABS:
                return emitInvokeFloatUnary(s, instr.opcode);

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

            case OpCode::INVOKE_INT_ADD: case OpCode::INVOKE_INT_SUB:
            case OpCode::INVOKE_INT_MUL: case OpCode::INVOKE_INT_DIV:
            case OpCode::INVOKE_INT_MOD:
            case OpCode::INVOKE_INT_NEG: case OpCode::INVOKE_INT_ABS:
            case OpCode::INVOKE_INT_EQUALS: case OpCode::INVOKE_INT_COMPARE:
            case OpCode::INVOKE_FLOAT_ADD: case OpCode::INVOKE_FLOAT_SUB:
            case OpCode::INVOKE_FLOAT_MUL: case OpCode::INVOKE_FLOAT_DIV:
            case OpCode::INVOKE_FLOAT_NEG: case OpCode::INVOKE_FLOAT_ABS:
            case OpCode::INVOKE_FLOAT_EQUALS: case OpCode::INVOKE_FLOAT_COMPARE:
                return emitInvokePrimitiveOps(s, instr);

            default:
                return false;
        }
    }
}
