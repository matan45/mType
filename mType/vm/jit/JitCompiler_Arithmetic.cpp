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

    static bool emitDivIntOp(JitEmissionState& s)
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
        Gp raxReg = cc.new_gp64();
        Gp rdxReg = cc.new_gp64();
        cc.mov(raxReg, left);
        cc.cqo(rdxReg, raxReg);
        cc.idiv(rdxReg, raxReg, right);
        s.slotTypes.push_back(SlotType::INT);
        publishGpHint(s, s.stackDepth - 1, raxReg);
        return true;
    }

    static bool emitUnaryIntOps(JitEmissionState& s,
                                 const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-211: in-place unary on TOS — consume into reg, op, publish.
        auto& cc = s.cc;
        Gp tmp = consumeGpHint(s, s.stackDepth - 1);
        switch (instr.opcode)
        {
            case OpCode::NEG:                                           cc.neg(tmp); break;
            case OpCode::INC:                                           cc.inc(tmp); break;
            case OpCode::DEC:                                           cc.dec(tmp); break;
            case OpCode::BITWISE_NOT_OP: case OpCode::BITWISE_NOT_INT:  cc.not_(tmp); break;
            default: return false;
        }
        publishGpHint(s, s.stackDepth - 1, tmp);
        return true;
    }

    static bool emitBitwiseIntOps(JitEmissionState& s,
                                   const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        // Accept both the generic _OP form and the INT-specialized form; the
        // INT variant carries the same semantics — compiler/runtime has just
        // proven the operand types are INT at the bytecode level.
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

        // Read the coherent stack slots directly. Bitwise ops are not hot
        // enough to need the virtual-register hint path, and the direct loads
        // mirror the older primitive-method emitters below.
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

    static bool emitShiftOp(JitEmissionState& s,
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

        // MYT-211: peephole — when the right operand is a known constant in
        // [0, 63] (as recorded by publishConstHint from PUSH_INT), skip the
        // runtime range check and emit the shift with an immediate operand.
        // Bitwise tight loops shift by literal 1/2/3 every iteration; this
        // eliminates 3+ never-taken branches per iter.
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
                cc.sal(left, asmjit::Imm(static_cast<uint32_t>(shamt)));
            else
                cc.sar(left, asmjit::Imm(static_cast<uint32_t>(shamt)));
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

    static bool emitGenericAddSubMul(JitEmissionState& s,
                                      const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-211: generic ADD/SUB/MUL handles the boxed mixed-type path via
        // emitGenericBinop and reads stackBase via Mem(...). Not hint-aware.
        flushAllHints(s);
        auto& cc = s.cc;
        SlotType rType = popType(s), lType = popType(s);
        s.stackDepth--;

        // MYT-254: for ADD with any non-numeric-primitive operand, route to
        // emitGenericBinop. The original code path called emitEnsureUnboxed
        // with target=INT, which routes the value through jit_unbox_int —
        // returning 0 for STRING/OBJECT/etc. The subsequent INT+INT path
        // then computes 0+0 (or 0+rhs), producing junk INT that flows into
        // downstream consumers (e.g. `new String(...)` with the int payload
        // as if it were a string). For ADD, route through jit_generic_add
        // which handles string concat properly. For SUB/MUL with non-numeric
        // operands, also route to generic so the helper's typed throw
        // surfaces (instead of the silent 0 + 0 result).
        //
        // Numeric-feedback specialization is preserved: when both operands
        // are BOXED but type feedback dominates as INT/FLOAT, we keep the
        // fast unbox path. For STRING/OBJECT/ARRAY/VALUE_OBJECT/STACK_OBJECT
        // (the boxed-but-not-numeric set) we always go generic.
        auto isNonNumericBoxed = [](SlotType t) {
            return t == SlotType::STRING || t == SlotType::OBJECT
                || t == SlotType::ARRAY  || t == SlotType::VALUE_OBJECT
                || t == SlotType::STACK_OBJECT;
        };
        const bool eitherStringy = isNonNumericBoxed(lType) || isNonNumericBoxed(rType);
        bool feedbackSaysNumeric = false;
        if (s.typeFeedback && s.typeFeedback->shouldSpecialize(s.currentIP))
        {
            auto [lt, rt] = s.typeFeedback->getDominantTypes(s.currentIP);
            feedbackSaysNumeric =
                (lt == ic::ObservedType::INT || lt == ic::ObservedType::FLOAT) &&
                (rt == ic::ObservedType::INT || rt == ic::ObservedType::FLOAT);
        }
        const bool eitherUnknownBoxed =
            (lType == SlotType::BOXED || rType == SlotType::BOXED) && !feedbackSaysNumeric;
        if (eitherStringy || (instr.opcode == OpCode::ADD && eitherUnknownBoxed))
        {
            uint64_t fn;
            if (instr.opcode == OpCode::ADD) fn = (uint64_t)jit_generic_add;
            else if (instr.opcode == OpCode::SUB) fn = (uint64_t)jit_generic_sub;
            else fn = (uint64_t)jit_generic_mul;
            emitGenericBinop(s, fn, lType, rType);
            return true;
        }

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
        // MYT-211: see emitGenericAddSubMul.
        flushAllHints(s);
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

    static bool emitLogicalOps(JitEmissionState& s,
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

        if (op == OpCode::INVOKE_INT_EQUALS ||
            op == OpCode::INVOKE_INT_LESS_THAN ||
            op == OpCode::INVOKE_INT_LESS_EQUAL ||
            op == OpCode::INVOKE_INT_GREATER_THAN ||
            op == OpCode::INVOKE_INT_GREATER_EQUAL)
        {
            Gp left = cc.new_gp64();
            Gp right = cc.new_gp64();
            cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
            Gp result = cc.new_gp64();
            cc.xor_(result, result);
            cc.cmp(left, right);
            switch (op)
            {
                case OpCode::INVOKE_INT_EQUALS:        cc.sete(result.r8()); break;
                case OpCode::INVOKE_INT_LESS_THAN:     cc.setl(result.r8()); break;
                case OpCode::INVOKE_INT_LESS_EQUAL:    cc.setle(result.r8()); break;
                case OpCode::INVOKE_INT_GREATER_THAN:  cc.setg(result.r8()); break;
                case OpCode::INVOKE_INT_GREATER_EQUAL: cc.setge(result.r8()); break;
                default: break;
            }
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

        if (op == OpCode::INVOKE_FLOAT_LESS_THAN ||
            op == OpCode::INVOKE_FLOAT_LESS_EQUAL ||
            op == OpCode::INVOKE_FLOAT_GREATER_THAN ||
            op == OpCode::INVOKE_FLOAT_GREATER_EQUAL)
        {
            // ucomisd sets unordered (PF=1) on NaN; all comparisons return
            // false on NaN to match interpreter (a < NaN, a > NaN both false).
            Vec left = cc.new_xmm();
            Vec right = cc.new_xmm();
            cc.movsd(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.movsd(right, Mem(s.stackBase, s.stackDepth * 8));
            // For LESS_*, swap operands and use seta/setae (which are NaN-safe).
            // For GREATER_*, use seta/setae directly on (left, right).
            bool swap = (op == OpCode::INVOKE_FLOAT_LESS_THAN ||
                         op == OpCode::INVOKE_FLOAT_LESS_EQUAL);
            if (swap) cc.ucomisd(right, left);
            else      cc.ucomisd(left, right);
            Gp result = cc.new_gp64();
            cc.xor_(result, result);
            // seta = ZF=0 && CF=0 (strict greater, false on NaN/equal)
            // setae = CF=0 (greater-or-equal, false on NaN)
            bool strict = (op == OpCode::INVOKE_FLOAT_LESS_THAN ||
                           op == OpCode::INVOKE_FLOAT_GREATER_THAN);
            if (strict) cc.seta(result.r8());
            else        cc.setae(result.r8());
            cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
            s.slotTypes.push_back(SlotType::BOOL);
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

    // MYT-155: inline accessors for boxed primitives. Pop boxed receiver,
    // ensure-unboxed at the receiver slot (calls jit_unbox_int / jit_unbox_float
    // which extract field 0 from ObjectInstance/ValueObject), push primitive
    // slot type. Net stackDepth change is 0 (in-place unbox).
    static bool emitInvokeIntGetValue(JitEmissionState& s)
    {
        SlotType rType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth - 1, rType, SlotType::INT);
        s.slotTypes.push_back(SlotType::INT);
        return true;
    }

    static bool emitInvokeFloatGetValue(JitEmissionState& s)
    {
        SlotType rType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth - 1, rType, SlotType::FLOAT);
        s.slotTypes.push_back(SlotType::FLOAT);
        return true;
    }

    static bool emitInvokeBoolGetValue(JitEmissionState& s)
    {
        SlotType rType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth - 1, rType, SlotType::BOOL);
        s.slotTypes.push_back(SlotType::BOOL);
        return true;
    }

    // INVOKE_BOOL_AND/OR/XOR/EQUALS: pop arg + receiver, normalize both to
    // raw bool (jit_unbox_int treats bool as 0/1 in int64), inline-emit the
    // op, push BOOL. Hint cache flushed at the dispatch level (the emitter
    // reads stackBase memory directly).
    static bool emitInvokeBoolBinary(JitEmissionState& s, OpCode op)
    {
        auto& cc = s.cc;
        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::BOOL);
        emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::BOOL);
        Gp left = cc.new_gp64();
        Gp right = cc.new_gp64();
        cc.mov(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        cc.mov(right, Mem(s.stackBase, s.stackDepth * 8));
        // Normalize each operand to {0,1}: TEST + SETNE collapses any non-zero
        // int64 into 1 so OR/XOR over arbitrary truthy ints stay correct.
        Gp nl = cc.new_gp64();
        Gp nr = cc.new_gp64();
        cc.xor_(nl, nl);
        cc.test(left, left);
        cc.setne(nl.r8());
        cc.xor_(nr, nr);
        cc.test(right, right);
        cc.setne(nr.r8());
        switch (op)
        {
            case OpCode::INVOKE_BOOL_AND:    cc.and_(nl, nr); break;
            case OpCode::INVOKE_BOOL_OR:     cc.or_(nl, nr); break;
            case OpCode::INVOKE_BOOL_XOR:    cc.xor_(nl, nr); break;
            case OpCode::INVOKE_BOOL_EQUALS:
            {
                Gp eq = cc.new_gp64();
                cc.xor_(eq, eq);
                cc.cmp(nl, nr);
                cc.sete(eq.r8());
                cc.mov(nl, eq);
                break;
            }
            default: return false;
        }
        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), nl);
        s.slotTypes.push_back(SlotType::BOOL);
        return true;
    }

    // INVOKE_BOOL_NOT: pop receiver, push !receiver as BOOL.
    static bool emitInvokeBoolNot(JitEmissionState& s)
    {
        auto& cc = s.cc;
        SlotType rType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth - 1, rType, SlotType::BOOL);
        Gp val = cc.new_gp64();
        Gp result = cc.new_gp64();
        cc.mov(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
        cc.xor_(result, result);
        cc.test(val, val);
        cc.sete(result.r8());
        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
        s.slotTypes.push_back(SlotType::BOOL);
        return true;
    }

    // INVOKE_STRING_LENGTH / IS_EMPTY: pop boxed receiver, call helper that
    // reads field 0 / raw-string content, push int (length) or bool (isEmpty).
    // Receiver lives on boxedBase (boxed-mode trigger fires for these opcodes).
    static bool emitInvokeStringUnary(JitEmissionState& s, OpCode op)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        SlotType rType = popType(s);
        // Receiver is a boxed Value at boxedBase[stackDepth-1].
        Gp recvAddr = cc.new_gp64();
        cc.lea(recvAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        InvokeNode* inv;
        uint64_t fn = (op == OpCode::INVOKE_STRING_LENGTH)
            ? reinterpret_cast<uint64_t>(jit_invoke_string_length)
            : reinterpret_cast<uint64_t>(jit_invoke_string_is_empty);
        cc.invoke(Out(inv), fn,
                  FuncSignature::build<int64_t, const value::Value*>());
        inv->set_arg(0, recvAddr);
        Gp ret = cc.new_gp64();
        inv->set_ret(0, ret);
        // Destroy the boxed receiver in place (it's being consumed) and write
        // the raw int/bool result into stackBase[stackDepth-1].
        InvokeNode* dest;
        cc.invoke(Out(dest), reinterpret_cast<uint64_t>(jit_value_destroy),
                  FuncSignature::build<void, value::Value*>());
        dest->set_arg(0, recvAddr);
        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), ret);
        s.slotTypes.push_back(op == OpCode::INVOKE_STRING_LENGTH
                                ? SlotType::INT : SlotType::BOOL);
        (void)rType;
        return true;
    }

    // INVOKE_STRING_EQUALS: pop arg + receiver (both boxed), call helper, push BOOL.
    static bool emitInvokeStringEquals(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        Gp argAddr = cc.new_gp64();
        cc.lea(argAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp recvAddr = cc.new_gp64();
        cc.lea(recvAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_invoke_string_equals),
                  FuncSignature::build<int64_t, const value::Value*, const value::Value*>());
        inv->set_arg(0, recvAddr);
        inv->set_arg(1, argAddr);
        Gp ret = cc.new_gp64();
        inv->set_ret(0, ret);
        // Destroy both boxed Values (consumed) before overwriting their slot.
        InvokeNode* dArg;
        cc.invoke(Out(dArg), reinterpret_cast<uint64_t>(jit_value_destroy),
                  FuncSignature::build<void, value::Value*>());
        dArg->set_arg(0, argAddr);
        InvokeNode* dRecv;
        cc.invoke(Out(dRecv), reinterpret_cast<uint64_t>(jit_value_destroy),
                  FuncSignature::build<void, value::Value*>());
        dRecv->set_arg(0, recvAddr);
        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), ret);
        s.slotTypes.push_back(SlotType::BOOL);
        (void)rType; (void)lType;
        return true;
    }

    // INVOKE_STRING_CONCAT: pop arg + receiver (both boxed), allocate the
    // concatenation result via jit_invoke_string_concat which interns into
    // the StringPool when possible. Result is a boxed STRING/INTERNED_STRING
    // Value written back to boxedBase[stackDepth-1].
    static bool emitInvokeStringConcat(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);
        Gp argAddr = cc.new_gp64();
        cc.lea(argAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp recvAddr = cc.new_gp64();
        cc.lea(recvAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        // jit_invoke_string_concat reads receiver+arg, destroys *dest in place,
        // and writes the new boxed result. dest = receiver slot (in-place).
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_invoke_string_concat),
                  FuncSignature::build<void, value::Value*, const value::Value*, const value::Value*>());
        inv->set_arg(0, recvAddr);  // dest
        inv->set_arg(1, recvAddr);  // receiver (read before destroy)
        inv->set_arg(2, argAddr);
        // Destroy the consumed arg slot.
        InvokeNode* dArg;
        cc.invoke(Out(dArg), reinterpret_cast<uint64_t>(jit_value_destroy),
                  FuncSignature::build<void, value::Value*>());
        dArg->set_arg(0, argAddr);
        s.slotTypes.push_back(SlotType::BOXED);
        (void)rType; (void)lType;
        return true;
    }

    static bool emitInvokePrimitiveOps(JitEmissionState& s,
                                        const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-211: INVOKE_INT_*/INVOKE_FLOAT_* emitters read stackBase memory
        // directly and aren't hint-aware. Flush at the dispatch level so any
        // prior cached value is materialized to memory before they run.
        flushAllHints(s);
        switch (instr.opcode)
        {
            case OpCode::INVOKE_INT_ADD: case OpCode::INVOKE_INT_SUB:
            case OpCode::INVOKE_INT_MUL: case OpCode::INVOKE_INT_DIV:
            case OpCode::INVOKE_INT_MOD:
            case OpCode::INVOKE_INT_EQUALS: case OpCode::INVOKE_INT_COMPARE:
            case OpCode::INVOKE_INT_LESS_THAN: case OpCode::INVOKE_INT_LESS_EQUAL:
            case OpCode::INVOKE_INT_GREATER_THAN: case OpCode::INVOKE_INT_GREATER_EQUAL:
                return emitInvokeIntBinary(s, instr.opcode);

            case OpCode::INVOKE_INT_NEG: case OpCode::INVOKE_INT_ABS:
                return emitInvokeIntUnary(s, instr.opcode);

            case OpCode::INVOKE_INT_GET_VALUE:
                return emitInvokeIntGetValue(s);

            case OpCode::INVOKE_FLOAT_ADD: case OpCode::INVOKE_FLOAT_SUB:
            case OpCode::INVOKE_FLOAT_MUL: case OpCode::INVOKE_FLOAT_DIV:
            case OpCode::INVOKE_FLOAT_EQUALS: case OpCode::INVOKE_FLOAT_COMPARE:
            case OpCode::INVOKE_FLOAT_LESS_THAN: case OpCode::INVOKE_FLOAT_LESS_EQUAL:
            case OpCode::INVOKE_FLOAT_GREATER_THAN: case OpCode::INVOKE_FLOAT_GREATER_EQUAL:
                return emitInvokeFloatBinary(s, instr.opcode);

            case OpCode::INVOKE_FLOAT_NEG: case OpCode::INVOKE_FLOAT_ABS:
                return emitInvokeFloatUnary(s, instr.opcode);

            case OpCode::INVOKE_FLOAT_GET_VALUE:
                return emitInvokeFloatGetValue(s);

            case OpCode::INVOKE_BOOL_GET_VALUE:
                return emitInvokeBoolGetValue(s);

            case OpCode::INVOKE_BOOL_AND:
            case OpCode::INVOKE_BOOL_OR:
            case OpCode::INVOKE_BOOL_XOR:
            case OpCode::INVOKE_BOOL_EQUALS:
                return emitInvokeBoolBinary(s, instr.opcode);
            case OpCode::INVOKE_BOOL_NOT:
                return emitInvokeBoolNot(s);

            case OpCode::INVOKE_STRING_LENGTH:
            case OpCode::INVOKE_STRING_IS_EMPTY:
                return emitInvokeStringUnary(s, instr.opcode);
            case OpCode::INVOKE_STRING_EQUALS:
                return emitInvokeStringEquals(s);
            case OpCode::INVOKE_STRING_CONCAT:
                return emitInvokeStringConcat(s);

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
                // {ADD,SUB,MUL}_INT. De-fuse at JIT time; the JIT machine-code
                // output is the same as the unfused sequence.
                bytecode::BytecodeProgram::Instruction load1(
                    OpCode::LOAD_LOCAL, instr.operands[0]);
                bytecode::BytecodeProgram::Instruction load2(
                    OpCode::LOAD_LOCAL, instr.operands[1]);
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
                    OpCode::STORE_LOCAL, instr.operands[0]);
                return emitControlFlowOps(s, store, nullptr);
            }

            case OpCode::ADD_INT_CONST:
            {
                // MYT-198: de-fuse at JIT time — emit equivalent of PUSH_INT
                // (from fusedSlot) + ADD_INT. Implemented inline rather than
                // through emitSimpleIntArithOps since the right operand is an
                // immediate, not a stack slot.
                // MYT-201: fused state lives on the side table; bail on
                // compile failure if the entry is missing (would indicate
                // an inconsistent program — should never happen in practice).
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
