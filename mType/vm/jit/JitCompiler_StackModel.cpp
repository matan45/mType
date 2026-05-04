#include "JitEmissionState.hpp"
#include <cstddef>
#include <cstdint>
#include <asmjit/x86.h>

// MYT-211: virtual-register operand-stack helpers. See SlotHint in
// JitEmissionState.hpp for the contract. The cache is opt-in and only
// activates in the !usesBoxedTypes path — boxed-mode emission stays
// byte-identical to its prior memory-only behavior.
namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;

    static SlotHint& slotAt(JitEmissionState& s, int stackIdx)
    {
        if (static_cast<size_t>(stackIdx) >= s.slotHints.size())
            s.slotHints.resize(stackIdx + 1);
        return s.slotHints[stackIdx];
    }

    // MYT-211: writes the published value to stackBase[stackIdx*8] in BOTH
    // boxed and non-boxed modes (primitives on the operand stack always live
    // in stackBase, regardless of usesBoxedTypes), AND records the virtreg
    // as a hint so the next consumer can skip the memory read. The memory
    // write keeps the slot coherent for any non-hint-aware emit that runs
    // before the next consumer (which calls flushAllHints at entry, but
    // that's a no-op when memory is already coherent).
    void publishGpHint(JitEmissionState& s, int stackIdx, Gp reg)
    {
        s.cc.mov(Mem(s.stackBase, stackIdx * 8), reg);
        SlotHint& h = slotAt(s, stackIdx);
        h.gp = reg;
        h.xmm = Vec();
        h.valid = true;
        h.dirty = false;        // memory is coherent (we just wrote it)
        h.isXmm = false;
        h.isConstant = false;
    }

    void publishXmmHint(JitEmissionState& s, int stackIdx, Vec reg, bool /*dirty*/)
    {
        s.cc.movsd(Mem(s.stackBase, stackIdx * 8), reg);
        SlotHint& h = slotAt(s, stackIdx);
        h.gp = Gp();
        h.xmm = reg;
        h.valid = true;
        h.dirty = false;
        h.isXmm = true;
        h.isConstant = false;
    }

    // MYT-211: record-only — caller has already emitted the memory write,
    // we just snapshot the producing virtreg so the next consumer can reuse
    // it without re-reading memory. Used by emitUnbox in the boxed-mode
    // LOAD_LOCAL/LOAD_VAR paths, where the unbox helper's return value lands
    // in stackBase via cc.mov(Mem, ret_reg) and we want to plumb ret_reg
    // into the cache without emitting a redundant store.
    void recordGpHint(JitEmissionState& s, int stackIdx, Gp reg)
    {
        // Boxed-mode shares the same slotHints vector — the unboxed primitive
        // still lives in stackBase[stackIdx*8] regardless of usesBoxedTypes,
        // so no mode-specific branch is needed here.
        SlotHint& h = slotAt(s, stackIdx);
        h.gp = reg;
        h.xmm = Vec();
        h.valid = true;
        h.dirty = false;
        h.isXmm = false;
        h.isConstant = false;
    }

    void recordXmmHint(JitEmissionState& s, int stackIdx, Vec reg)
    {
        SlotHint& h = slotAt(s, stackIdx);
        h.gp = Gp();
        h.xmm = reg;
        h.valid = true;
        h.dirty = false;
        h.isXmm = true;
        h.isConstant = false;
    }

    // MYT-211: records a known-constant value at the slot. Memory is already
    // coherent (PUSH_INT/PUSH_BOOL write the literal before calling this), so
    // dirty stays false. The constant tag enables peephole folds — e.g.
    // emitShiftOp can skip the runtime range check when isConstant &&
    // constValue ∈ [0, 63]. Skipped in boxed mode since boxed-mode shifts
    // route through emitEnsureUnboxed (which writes stackBase) and the const
    // identity is lost there.
    void publishConstHint(JitEmissionState& s, int stackIdx, int64_t value)
    {
        if (s.usesBoxedTypes) return;
        SlotHint& h = slotAt(s, stackIdx);
        h.constValue = value;
        h.valid = true;
        h.dirty = false;
        h.isXmm = false;
        h.isConstant = true;
    }

    // MYT-211: prefer a cached virtreg when one is published, else load from
    // memory. Constant hints materialize into a fresh gp via cc.mov(reg, imm)
    // so the caller gets a uniform Gp result. Always invalidates the slot's
    // hint after consumption — the next read of the same slot must either
    // see a fresh publish or fall back to memory (memory stays coherent
    // because publishGpHint also writes it).
    Gp consumeGpHint(JitEmissionState& s, int stackIdx)
    {
        auto& cc = s.cc;
        // Cache works in BOTH boxed and non-boxed modes — the hint records a
        // virtreg holding the unboxed primitive that lives in stackBase, and
        // that's the same in either mode.
        if (static_cast<size_t>(stackIdx) < s.slotHints.size())
        {
            SlotHint& h = s.slotHints[stackIdx];
            if (h.valid && !h.isXmm)
            {
                if (h.isConstant)
                {
                    Gp reg = cc.new_gp64();
                    cc.mov(reg, h.constValue);
                    h.valid = false; h.isConstant = false;
                    return reg;
                }
                Gp reg = h.gp;
                h.valid = false;
                return reg;
            }
        }
        Gp reg = cc.new_gp64();
        cc.mov(reg, Mem(s.stackBase, stackIdx * 8));
        return reg;
    }

    Vec consumeXmmHint(JitEmissionState& s, int stackIdx)
    {
        auto& cc = s.cc;
        if (static_cast<size_t>(stackIdx) < s.slotHints.size())
        {
            SlotHint& h = s.slotHints[stackIdx];
            if (h.valid && h.isXmm)
            {
                Vec reg = h.xmm;
                h.valid = false;
                return reg;
            }
        }
        Vec reg = cc.new_xmm();
        cc.movsd(reg, Mem(s.stackBase, stackIdx * 8));
        return reg;
    }

    void flushSlot(JitEmissionState& s, int stackIdx)
    {
        if (s.usesBoxedTypes) return;
        if (static_cast<size_t>(stackIdx) >= s.slotHints.size()) return;
        SlotHint& h = s.slotHints[stackIdx];
        if (!h.valid || !h.dirty) return;
        auto& cc = s.cc;
        if (h.isConstant)
        {
            Gp reg = cc.new_gp64();
            cc.mov(reg, h.constValue);
            cc.mov(Mem(s.stackBase, stackIdx * 8), reg);
        }
        else if (h.isXmm)
        {
            cc.movsd(Mem(s.stackBase, stackIdx * 8), h.xmm);
        }
        else
        {
            cc.mov(Mem(s.stackBase, stackIdx * 8), h.gp);
        }
        h.dirty = false;
    }

    // Flush every dirty slot to memory and clear all hints. Called before any
    // non-hint-aware emitter runs (so it sees memory-coherent state) and at
    // every label bind in the codegen loop (so a forward-jump target can rely
    // on memory). After this returns, the cache is empty and every slot's
    // memory is up to date.
    void flushAllHints(JitEmissionState& s)
    {
        if (s.usesBoxedTypes) { s.slotHints.clear(); return; }
        for (size_t i = 0; i < s.slotHints.size(); ++i)
            flushSlot(s, static_cast<int>(i));
        s.slotHints.clear();
    }

    // Drop all hints without flushing. Used right after cc.bind(label) since a
    // forward-jump source has its own (possibly different) cache state — the
    // bind's downstream emitters must reload from memory. Memory must already
    // be coherent (the bind site flushed beforehand).
    void invalidateAllHints(JitEmissionState& s)
    {
        s.slotHints.clear();
    }
}
