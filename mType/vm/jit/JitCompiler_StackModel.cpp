#include "JitEmissionState.hpp"
#include <cstddef>
#include <cstdint>
#include <asmjit/x86.h>

// MYT-211: virtual-register operand-stack helpers. See SlotHint in
// JitEmissionState.hpp for the contract. Deferred write-back only activates
// in the unboxed pipeline; boxed-mode hints are always memory-coherent.
namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;

    static bool validStackIndex(JitEmissionState& s, int stackIdx)
    {
        if (stackIdx < 0 ||
            static_cast<size_t>(stackIdx) >= s.operandStackCapacity)
        {
            s.compileFailed = true;
            return false;
        }
        return true;
    }

    static SlotHint* slotAt(JitEmissionState& s, int stackIdx)
    {
        if (!validStackIndex(s, stackIdx)) return nullptr;
        const size_t index = static_cast<size_t>(stackIdx);
        if (index >= s.slotHints.size())
            s.slotHints.resize(index + 1);
        return &s.slotHints[index];
    }

    // MYT-211: unboxed producers keep their result register-resident and defer
    // the stack write until a helper/control-flow boundary. Boxed-mode code
    // retains coherent memory because those emitters routinely cross helpers
    // and inspect stackBase directly.
    void publishGpHint(JitEmissionState& s, int stackIdx, Gp reg)
    {
        SlotHint* h = slotAt(s, stackIdx);
        if (!h) return;
        const bool dirty = !s.usesBoxedTypes;
        if (!dirty)
            s.cc.mov(Mem(s.stackBase, stackIdx * 8), reg);
        h->gp = reg;
        h->xmm = Vec();
        h->valid = true;
        h->dirty = dirty;
        h->isXmm = false;
        h->isConstant = false;
    }

    void publishXmmHint(JitEmissionState& s, int stackIdx, Vec reg, bool dirty)
    {
        SlotHint* h = slotAt(s, stackIdx);
        if (!h) return;
        // flushSlot intentionally has no boxed-mode work, so boxed emitters
        // always publish a coherent stack slot.
        dirty = dirty && !s.usesBoxedTypes;
        if (!dirty)
            s.cc.movsd(Mem(s.stackBase, stackIdx * 8), reg);
        h->gp = Gp();
        h->xmm = reg;
        h->valid = true;
        h->dirty = dirty;
        h->isXmm = true;
        h->isConstant = false;
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
        SlotHint* h = slotAt(s, stackIdx);
        if (!h) return;
        h->gp = reg;
        h->xmm = Vec();
        h->valid = true;
        h->dirty = false;
        h->isXmm = false;
        h->isConstant = false;
    }

    void recordXmmHint(JitEmissionState& s, int stackIdx, Vec reg)
    {
        SlotHint* h = slotAt(s, stackIdx);
        if (!h) return;
        h->gp = Gp();
        h->xmm = reg;
        h->valid = true;
        h->dirty = false;
        h->isXmm = true;
        h->isConstant = false;
    }

    // MYT-211: records a known-constant value at the slot without eagerly
    // materializing stack memory. The constant tag enables peephole folds —
    // emitShiftOp can skip the runtime range check when isConstant &&
    // constValue ∈ [0, 63]. Skipped in boxed mode since boxed-mode shifts
    // route through emitEnsureUnboxed (which writes stackBase) and the const
    // identity is lost there.
    void publishConstHint(JitEmissionState& s, int stackIdx, int64_t value)
    {
        if (!validStackIndex(s, stackIdx)) return;
        if (s.usesBoxedTypes) return;
        SlotHint* h = slotAt(s, stackIdx);
        if (!h) return;
        h->constValue = value;
        h->valid = true;
        h->dirty = true;
        h->isXmm = false;
        h->isConstant = true;
    }

    // MYT-211: prefer a cached virtreg when one is published, else load from
    // memory. Constant hints materialize into a fresh gp via cc.mov(reg, imm)
    // so the caller gets a uniform Gp result. Always invalidates the slot's
    // hint after consumption. A dirty consumed slot must then be overwritten,
    // discarded, or republished by the caller.
    Gp consumeGpHint(JitEmissionState& s, int stackIdx)
    {
        auto& cc = s.cc;
        if (!validStackIndex(s, stackIdx))
        {
            Gp reg = cc.new_gp64();
            cc.xor_(reg, reg);
            return reg;
        }
        // Cache works in BOTH boxed and non-boxed modes — the hint records a
        // virtreg holding the unboxed primitive that lives in stackBase, and
        // that's the same in either mode.
        if (static_cast<size_t>(stackIdx) < s.slotHints.size())
        {
            SlotHint& h = s.slotHints[stackIdx];
            if (h.valid)
            {
                if (h.isConstant)
                {
                    Gp reg = cc.new_gp64();
                    cc.mov(reg, h.constValue);
                    h.valid = false; h.isConstant = false;
                    return reg;
                }
                if (h.isXmm)
                {
                    // Preserve the raw 64-bit payload if a FLOAT producer is
                    // consumed through the GP representation path.
                    Gp reg = cc.new_gp64();
                    cc.movq(reg, h.xmm);
                    h.valid = false;
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
        if (!validStackIndex(s, stackIdx))
        {
            Vec reg = cc.new_xmm();
            cc.xorpd(reg, reg);
            return reg;
        }
        if (static_cast<size_t>(stackIdx) < s.slotHints.size())
        {
            SlotHint& h = s.slotHints[stackIdx];
            if (h.valid)
            {
                if (h.isXmm)
                {
                    Vec reg = h.xmm;
                    h.valid = false;
                    return reg;
                }

                // GP hints can carry an IEEE-754 payload. Transfer the bits
                // directly instead of forcing a stale memory round-trip; this
                // is not a numeric INT-to-FLOAT conversion.
                Gp bits = h.gp;
                if (h.isConstant)
                {
                    bits = cc.new_gp64();
                    cc.mov(bits, h.constValue);
                    h.isConstant = false;
                }
                Vec reg = cc.new_xmm();
                cc.movq(reg, bits);
                h.valid = false;
                return reg;
            }
        }
        Vec reg = cc.new_xmm();
        cc.movsd(reg, Mem(s.stackBase, stackIdx * 8));
        return reg;
    }

    bool consumeIntConstantHint(JitEmissionState& s, int stackIdx,
                                int64_t& value)
    {
        if (!validStackIndex(s, stackIdx)) return false;
        if (static_cast<size_t>(stackIdx) >= s.slotHints.size()) return false;
        SlotHint& h = s.slotHints[stackIdx];
        if (!h.valid || h.isXmm || !h.isConstant) return false;
        value = h.constValue;
        h.valid = false;
        h.isConstant = false;
        return true;
    }

    void flushSlot(JitEmissionState& s, int stackIdx)
    {
        if (!validStackIndex(s, stackIdx)) return;
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
