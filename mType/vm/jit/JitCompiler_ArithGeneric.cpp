#include "JitCompiler.hpp"
#include <cstddef>
#include <cstdint>
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

    // Defined in JitCompiler_ArithFloat.cpp.
    bool emitFloatArithOps(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr);

    namespace
    {
        bool emitGenericAddSubMul(JitEmissionState& s,
                                   const bytecode::BytecodeProgram::Instruction& instr)
        {
            // MYT-211: generic ADD/SUB/MUL handles the boxed mixed-type path
            // via emitGenericBinop and reads stackBase via Mem(...). Not
            // hint-aware.
            flushAllHints(s);
            auto& cc = s.cc;
            SlotType rType = popType(s), lType = popType(s);
            s.stackDepth--;

            // MYT-254: for ADD with any non-numeric-primitive operand, route
            // to emitGenericBinop. The original code path called
            // emitEnsureUnboxed with target=INT, which routes the value
            // through jit_unbox_int — returning 0 for STRING/OBJECT/etc. For
            // ADD, route through jit_generic_add which handles string concat
            // properly. For SUB/MUL with non-numeric operands, route to
            // generic so the helper's typed throw surfaces (instead of the
            // silent 0 + 0 result). Numeric-feedback specialization is
            // preserved: when both operands are BOXED but type feedback
            // dominates as INT/FLOAT, we keep the fast unbox path.
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

            SlotType unboxTarget = (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
                ? SlotType::FLOAT : SlotType::INT;
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

        bool emitGenericDivMod(JitEmissionState& s,
                                const bytecode::BytecodeProgram::Instruction& instr)
        {
            // MYT-211: see emitGenericAddSubMul.
            flushAllHints(s);
            auto& cc = s.cc;
            SlotType rType = popType(s), lType = popType(s);
            s.stackDepth--;

            SlotType unboxTarget = (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
                ? SlotType::FLOAT : SlotType::INT;
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
                cc.jp(nz);   // unordered (NaN): skip zero trap
                cc.jne(nz);  // non-zero: skip zero trap
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
    }

    bool emitGenericArithmeticOps(JitEmissionState& s,
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
}
