#include "JitCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;

    namespace
    {
        // Returns true if comparison was fully handled (both-boxed EQ/NE).
        // Returns false if types were unboxed and caller should fall through
        // to primitive path.
        bool emitCmpBoxed(JitEmissionState& s, CmpOp kind, Gp result,
                          SlotType& lType, SlotType& rType)
        {
            // MYT-211: boxed-cmp path uses cc.invoke and reads boxed memory;
            // flush before the box reads. No-op when cache is empty.
            flushAllHints(s);
            auto& cc = s.cc;
            bool bothBoxed = isBoxedSlotType(lType) && isBoxedSlotType(rType);

            if (!bothBoxed)
            {
                SlotType target = !isBoxedSlotType(lType) ? lType : rType;
                if (target == SlotType::BOOL) target = SlotType::INT;
                emitEnsureUnboxed(s, s.stackDepth - 1, lType,
                    target == SlotType::FLOAT ? SlotType::FLOAT : SlotType::INT);
                emitEnsureUnboxed(s, s.stackDepth, rType,
                    target == SlotType::FLOAT ? SlotType::FLOAT : SlotType::INT);
                lType = target;
                rType = target;
                return false;
            }

            if (kind == CmpOp::EQ || kind == CmpOp::NE)
            {
                constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

                Gp leftAddr = cc.new_gp64();
                cc.lea(leftAddr, Mem(s.boxedBase,
                    static_cast<int32_t>((s.stackDepth - 1) * valueSize)));

                Gp rightAddr = cc.new_gp64();
                cc.lea(rightAddr, Mem(s.boxedBase,
                    static_cast<int32_t>(s.stackDepth * valueSize)));

                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_values_equal),
                          FuncSignature::build<int64_t, const value::Value*,
                                               const value::Value*>());
                inv->set_arg(0, leftAddr);
                inv->set_arg(1, rightAddr);
                inv->set_ret(0, result);

                if (kind == CmpOp::NE)
                    cc.xor_(result, 1);

                return true;
            }

            emitEnsureUnboxed(s, s.stackDepth - 1, lType, SlotType::INT);
            emitEnsureUnboxed(s, s.stackDepth, rType, SlotType::INT);
            lType = SlotType::INT;
            rType = SlotType::INT;
            return false;
        }

        void emitCmpPrimitive(JitEmissionState& s, CmpOp kind, Gp result,
                              SlotType lType, SlotType rType)
        {
            auto& cc = s.cc;

            if (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
            {
                Vec right = consumeXmmHint(s, s.stackDepth);
                Vec left = consumeXmmHint(s, s.stackDepth - 1);
                cc.ucomisd(left, right);
                switch (kind) {
                    case CmpOp::EQ: cc.sete(result.r8()); break;
                    case CmpOp::NE: cc.setne(result.r8()); break;
                    case CmpOp::LT: cc.setb(result.r8()); break;
                    case CmpOp::GT: cc.seta(result.r8()); break;
                    case CmpOp::LE: cc.setbe(result.r8()); break;
                    case CmpOp::GE: cc.setae(result.r8()); break;
                }
            }
            else
            {
                Gp right = consumeGpHint(s, s.stackDepth);
                Gp left = consumeGpHint(s, s.stackDepth - 1);
                cc.cmp(left, right);
                switch (kind) {
                    case CmpOp::EQ: cc.sete(result.r8()); break;
                    case CmpOp::NE: cc.setne(result.r8()); break;
                    case CmpOp::LT: cc.setl(result.r8()); break;
                    case CmpOp::GT: cc.setg(result.r8()); break;
                    case CmpOp::LE: cc.setle(result.r8()); break;
                    case CmpOp::GE: cc.setge(result.r8()); break;
                }
            }
        }
    }

    void emitGenericBinop(JitEmissionState& s, uint64_t helperFn,
                          SlotType lType, SlotType rType)
    {
        // MYT-211: this helper boxes operands via emitBoxOrCopy → cc.invoke,
        // and ends with another cc.invoke to the helper itself. Flush so the
        // box reads see the latest cached values.
        flushAllHints(s);
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        Gp leftAddr = cc.new_gp64();
        cc.lea(leftAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
        emitBoxOrCopy(s, leftAddr, s.stackDepth - 1, lType);

        Gp rightAddr = cc.new_gp64();
        cc.lea(rightAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)
                              + static_cast<int32_t>(valueSize)));
        emitBoxOrCopy(s, rightAddr, s.stackDepth, rType);

        Gp resultAddr = cc.new_gp64();
        cc.lea(resultAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)
                                + static_cast<int32_t>(2 * valueSize)));

        InvokeNode* inv;
        cc.invoke(Out(inv), helperFn,
                  FuncSignature::build<void, value::Value*, const value::Value*, const value::Value*>());
        inv->set_arg(0, resultAddr);
        inv->set_arg(1, leftAddr);
        inv->set_arg(2, rightAddr);

        // MYT-254: when either operand is string-like or BOXED, the helper
        // (jit_generic_add) may return a STRING value via the string-concat
        // path. emitUnbox via jit_unbox_int would silently return 0 for a
        // STRING, leaving the operand stack with garbage that downstream
        // ops (e.g. NEW String) would treat as the actual concat result.
        // Copy the result Value into boxedBase so the downstream BOXED
        // consumer sees the real concatenated string. Pure numeric mixed
        // cases (e.g. INT+FLOAT) keep the FLOAT/INT unbox path.
        const bool maybeStringConcat =
            lType == SlotType::STRING || rType == SlotType::STRING ||
            isBoxedSlotType(lType)    || isBoxedSlotType(rType);

        if (maybeStringConcat)
        {
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.boxedBase,
                                 static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
            InvokeNode* cpInv;
            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            cpInv->set_arg(0, destAddr);
            cpInv->set_arg(1, resultAddr);
            s.slotTypes.push_back(SlotType::BOXED);
        }
        else
        {
            SlotType resType = (lType == SlotType::FLOAT || rType == SlotType::FLOAT)
                             ? SlotType::FLOAT : SlotType::INT;
            emitUnbox(s, resultAddr, s.stackDepth - 1, resType);
            s.slotTypes.push_back(resType);
        }
    }

    void emitCmp(JitEmissionState& s, CmpOp kind)
    {
        auto& cc = s.cc;
        s.stackDepth--;
        SlotType rType = popType(s);
        SlotType lType = popType(s);

        Gp result = cc.new_gp64();
        cc.xor_(result, result);

        if (isBoxedSlotType(lType) || isBoxedSlotType(rType))
        {
            if (emitCmpBoxed(s, kind, result, lType, rType))
            {
                cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
                s.slotTypes.push_back(SlotType::BOOL);
                return;
            }
        }

        emitCmpPrimitive(s, kind, result, lType, rType);
        // MYT-211: write the boolean result via publishGpHint (always writes
        // stackBase). Same final memory state as the original cc.mov.
        s.slotTypes.push_back(SlotType::BOOL);
        publishGpHint(s, s.stackDepth - 1, result);
    }
}
