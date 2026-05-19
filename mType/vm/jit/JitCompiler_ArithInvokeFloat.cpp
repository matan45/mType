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

    // MYT-144 (Float specialization): bytecode-proven scalar ops on a boxed
    // Float receiver. ucomisd sets PF=1 on unordered (NaN) — comparisons
    // below mask through PF / use NaN-safe SETcc variants to match the
    // interpreter's "all-false on NaN" contract.
    bool emitInvokeFloatBinary(JitEmissionState& s, OpCode op)
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
            // Three-way w/ NaN -> 0.
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
            cc.seta(gt.r8());
            cc.setb(ltRaw.r8());
            cc.setnp(ordered.r8());
            cc.and_(ltRaw, ordered);
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
            // For LESS_*, swap operands so seta/setae (NaN-safe) work
            // directly. CF=1 on unordered forces both to 0 (false on NaN).
            Vec left = cc.new_xmm();
            Vec right = cc.new_xmm();
            cc.movsd(left, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.movsd(right, Mem(s.stackBase, s.stackDepth * 8));
            bool swap = (op == OpCode::INVOKE_FLOAT_LESS_THAN ||
                         op == OpCode::INVOKE_FLOAT_LESS_EQUAL);
            if (swap) cc.ucomisd(right, left);
            else      cc.ucomisd(left, right);
            Gp result = cc.new_gp64();
            cc.xor_(result, result);
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

    bool emitInvokeFloatUnary(JitEmissionState& s, OpCode op)
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
        else // INVOKE_FLOAT_ABS: clear sign bit via AND with 0x7FFF...FFFF.
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

    // MYT-155: inline accessor for boxed Float. jit_unbox_float extracts
    // field 0 from the ObjectInstance / ValueObject in place.
    bool emitInvokeFloatGetValue(JitEmissionState& s)
    {
        SlotType rType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth - 1, rType, SlotType::FLOAT);
        s.slotTypes.push_back(SlotType::FLOAT);
        return true;
    }
}
