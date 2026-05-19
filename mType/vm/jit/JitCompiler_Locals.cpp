#include "JitCompiler_ControlFlow.hpp"
#include <cstddef>
#include <cstdint>
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;

    void emitLoadLocal(JitEmissionState& s, size_t slot,
                       bool hasForcedType, SlotType forcedType)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        // MYT-163: remap the bytecode's logical slot into the caller's frame
        // when emission is inside an inlined callee. inlineLocalsBase == 0 at
        // the top level — the computation is a no-op for non-inline paths.
        const size_t physSlot = slot + s.inlineLocalsBase;
        SlotType lt = hasForcedType
            ? forcedType
            : (s.localTypes.count(physSlot) ? s.localTypes[physSlot] : SlotType::INT);

        if (s.usesBoxedTypes && isBoxedSlotType(lt))
        {
            flushAllHints(s);
            Gp src = cc.new_gp64();
            cc.lea(src, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            Gp dst = cc.new_gp64();
            cc.lea(dst, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            inv->set_arg(0, dst);
            inv->set_arg(1, src);

            // MYT-292: mirror the primitive payload to stackBase so primitive-
            // stack consumers (JUMP_IF_FALSE / JUMP_IF_TRUE / arith) see the
            // right value when the local holds a BOOL/INT stored via the boxed
            // path. Without this mirror, JUMP_IF_FALSE reads stale stackBase
            // and `if (helloOpen)` consistently branches false after tier-up.
            // Symmetric to MYT-154 / MYT-252.
            InvokeNode* unbox;
            cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                      FuncSignature::build<int64_t, const value::Value*>());
            unbox->set_arg(0, dst);
            Gp unboxed = cc.new_gp64();
            unbox->set_ret(0, unboxed);
            cc.mov(Mem(s.stackBase, s.stackDepth * 8), unboxed);
        }
        else if (s.usesBoxedTypes)
        {
            flushAllHints(s);
            Gp localAddr = cc.new_gp64();
            cc.lea(localAddr, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            if (lt == SlotType::FLOAT)
            {
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_float),
                          FuncSignature::build<double, const value::Value*>());
                inv->set_arg(0, localAddr);
                Vec val = cc.new_xmm();
                inv->set_ret(0, val);
                cc.movsd(Mem(s.stackBase, s.stackDepth * 8), val);
                // MYT-211: record the unboxed primitive in the cache so the
                // next arith/cmp consumer skips re-reading stackBase.
                recordXmmHint(s, s.stackDepth, val);
            }
            else
            {
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_int),
                          FuncSignature::build<int64_t, const value::Value*>());
                inv->set_arg(0, localAddr);
                Gp val = cc.new_gp64();
                inv->set_ret(0, val);
                cc.mov(Mem(s.stackBase, s.stackDepth * 8), val);
                recordGpHint(s, s.stackDepth, val);
            }
        }
        else
        {
            // MYT-211: load to a fresh virtreg, write to stack memory via
            // publishGpHint. With caching disabled, publishGpHint just emits
            // cc.mov(Mem, reg) — same generated code as the previous inline
            // form. The indirection lets a future cache layer record the
            // virtreg without changing this site.
            Gp tmp = cc.new_gp64();
            cc.mov(tmp, Mem(s.localsBase, static_cast<int32_t>(physSlot * 8)));
            s.slotTypes.push_back(lt);
            publishGpHint(s, s.stackDepth, tmp);
            s.stackDepth++;
            return;
        }
        s.slotTypes.push_back(lt);
        s.stackDepth++;
    }

    void emitStoreLocal(JitEmissionState& s, size_t slot,
                        bool hasForcedType, SlotType forcedType)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        SlotType tt = topType(s);
        SlotType storedType = hasForcedType ? forcedType : tt;
        // MYT-163: see emitLoadLocal — remap through inlineLocalsBase.
        const size_t physSlot = slot + s.inlineLocalsBase;

        if (s.usesBoxedTypes && isBoxedSlotType(tt) && !hasForcedType)
        {
            flushAllHints(s);
            Gp src = cc.new_gp64();
            cc.lea(src, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
            Gp dst = cc.new_gp64();
            cc.lea(dst, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            inv->set_arg(0, dst);
            inv->set_arg(1, src);
        }
        else if (s.usesBoxedTypes)
        {
            if (hasForcedType && isBoxedSlotType(tt))
                emitEnsureUnboxed(s, s.stackDepth - 1, tt, storedType);
            flushAllHints(s);
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            if (storedType == SlotType::FLOAT)
            {
                Vec val = cc.new_xmm();
                cc.movsd(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_float),
                          FuncSignature::build<void, value::Value*, double>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, val);
            }
            else
            {
                Gp val = cc.new_gp64();
                cc.mov(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                uint64_t fn = (storedType == SlotType::BOOL)
                    ? reinterpret_cast<uint64_t>(jit_box_bool)
                    : reinterpret_cast<uint64_t>(jit_box_int);
                InvokeNode* inv;
                cc.invoke(Out(inv), fn,
                          FuncSignature::build<void, value::Value*, int64_t>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, val);
            }
        }
        else
        {
            // MYT-211: consume + republish. Now safe in boxed mode after
            // publishGpHint was fixed to always write to stackBase.
            Gp val = consumeGpHint(s, s.stackDepth - 1);
            cc.mov(Mem(s.localsBase, static_cast<int32_t>(physSlot * 8)), val);
            publishGpHint(s, s.stackDepth - 1, val);
        }
        s.localTypes[physSlot] = storedType;
        s.arrayInfoCache.erase(static_cast<int>(physSlot + 10000));
    }

    // MYT-152: JIT emitter for LOAD_VAR. Globals and unqualified field lookups
    // have no compile-time primitive type, so the loaded Value is always
    // placed in the boxed operand stack. scanOpcodesForBoxedTypes forces
    // s.usesBoxedTypes = true whenever LOAD_VAR is present in the loop body;
    // the guard below bails cleanly if that invariant is ever violated.
    void emitLoadVar(JitEmissionState& s, uint32_t nameIndex)
    {
        if (!s.usesBoxedTypes) { s.compileFailed = true; return; }
        // MYT-211: flush is a no-op in boxed mode (cache stays empty there),
        // but kept for symmetry — if the cache invariant changes later this
        // call site stays correct.
        flushAllHints(s);
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase,
                         static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_load_var),
                  FuncSignature::build<void, value::Value*, JitContext*,
                                        uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);
        inv->set_arg(2, niReg);

        s.slotTypes.push_back(SlotType::BOXED);
        s.stackDepth++;
    }

    // MYT-152: JIT emitter for STORE_VAR. The runtime pops the value, writes
    // it to the global/field, then re-pushes the same bits (so expression-
    // context `int x = (a = 5)` works). We mirror emitStoreLocal's approach:
    // pass the top slot to jit_store_var and leave stackDepth / slotTypes
    // untouched, since the bits at top-of-stack are identical before and
    // after.
    void emitStoreVar(JitEmissionState& s, uint32_t nameIndex)
    {
        if (!s.usesBoxedTypes) { s.compileFailed = true; return; }
        flushAllHints(s);
        auto& cc = s.cc;

        SlotType valType = topType(s);
        Gp valAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, valType);
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_store_var),
                  FuncSignature::build<void, JitContext*, uint32_t,
                                        const value::Value*>());
        inv->set_arg(0, s.ctxPtr);
        inv->set_arg(1, niReg);
        inv->set_arg(2, valAddr);
    }

    // MYT-208: DECLARE_VAR — pop the top-of-stack value and register a new
    // global with that value. Stack effect: -1 (pure pop). The helper's
    // VariableDefinition ctor copies the Value internally; on the JIT side
    // we just need to drop the slot. Boxed slots get an explicit destroy so
    // their heap-ref refcount is released.
    void emitDeclareVar(JitEmissionState& s,
                        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (!s.usesBoxedTypes) { s.compileFailed = true; return; }
        flushAllHints(s);
        auto& cc = s.cc;
        uint32_t nameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        // operand[1] is the type-name index (currently unused at JIT level).
        uint8_t isFinal = (instr.numOperands() >= 3 && instr.inlineOperands[2] != 0) ? 1 : 0;

        SlotType valType = topType(s);
        Gp valAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, valType);
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));
        Gp finalReg = cc.new_gp64();
        cc.mov(finalReg, static_cast<int64_t>(isFinal));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_declare_var),
                  FuncSignature::build<void, JitContext*, uint32_t,
                                        const value::Value*, uint8_t>());
        inv->set_arg(0, s.ctxPtr);
        inv->set_arg(1, niReg);
        inv->set_arg(2, valAddr);
        inv->set_arg(3, finalReg);

        if (isBoxedSlotType(valType))
        {
            emitValueDestroy(s, s.stackDepth - 1);
        }
        popType(s);
        s.stackDepth--;
    }
}
