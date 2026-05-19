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

    // MYT-155: inline accessor for boxed Bool. jit_unbox_int extracts field
    // 0 (treats bool as 0/1 in int64) in place.
    bool emitInvokeBoolGetValue(JitEmissionState& s)
    {
        SlotType rType = popType(s);
        emitEnsureUnboxed(s, s.stackDepth - 1, rType, SlotType::BOOL);
        s.slotTypes.push_back(SlotType::BOOL);
        return true;
    }

    // INVOKE_BOOL_*: normalize each operand to {0,1} (TEST + SETNE collapses
    // any non-zero int64 into 1 so OR/XOR over arbitrary truthy ints stay
    // correct), then emit the op inline.
    bool emitInvokeBoolBinary(JitEmissionState& s, OpCode op)
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

    bool emitInvokeBoolNot(JitEmissionState& s)
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
}
