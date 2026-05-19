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

    // MYT-144: specialized primitive method opcodes — the bytecode compiler
    // proves a method call reduces to a scalar op on a boxed Int receiver.
    // emitEnsureUnboxed normalizes inputs to raw int64; result pushed as
    // raw primitive (lazy-rebox) per PrimitiveMethodExecutor's contract.
    bool emitInvokeIntBinary(JitEmissionState& s, OpCode op)
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

    bool emitInvokeIntUnary(JitEmissionState& s, OpCode op)
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

    // MYT-155: inline accessor for boxed Int. jit_unbox_int extracts field 0
    // from the ObjectInstance / ValueObject in place.
    bool emitInvokeIntGetValue(JitEmissionState& s)
    {
        SlotType rType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth - 1, rType, SlotType::INT);
        s.slotTypes.push_back(SlotType::INT);
        return true;
    }
}
