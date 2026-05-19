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

    void emitBox(JitEmissionState& s, Gp destAddr, int stackOff, SlotType type)
    {
        // MYT-211: emitBox reads stackBase memory and emits a helper invoke;
        // flush so the read picks up any register-cached value at stackOff.
        flushAllHints(s);
        auto& cc = s.cc;
        if (type == SlotType::FLOAT)
        {
            Vec val = cc.new_xmm();
            cc.movsd(val, Mem(s.stackBase, stackOff * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), (uint64_t)jit_box_float,
                      FuncSignature::build<void, value::Value*, double>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, val);
        }
        else
        {
            Gp val = cc.new_gp64();
            cc.mov(val, Mem(s.stackBase, stackOff * 8));
            uint64_t fn = (type == SlotType::BOOL) ? (uint64_t)jit_box_bool : (uint64_t)jit_box_int;
            InvokeNode* inv;
            cc.invoke(Out(inv), fn, FuncSignature::build<void, value::Value*, int64_t>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, val);
        }
    }

    void emitUnbox(JitEmissionState& s, Gp srcAddr, int stackOff, SlotType type)
    {
        // MYT-211: emitUnbox crosses a cc.invoke boundary; flush any pending
        // dirty cache slots so the helper sees coherent memory. After the
        // unbox, record the result virtreg as a hint so the next consumer
        // skips re-reading stackBase[stackOff*8].
        flushAllHints(s);
        auto& cc = s.cc;
        if (type == SlotType::FLOAT)
        {
            InvokeNode* inv;
            cc.invoke(Out(inv), (uint64_t)jit_unbox_float,
                      FuncSignature::build<double, const value::Value*>());
            inv->set_arg(0, srcAddr);
            Vec val = cc.new_xmm();
            inv->set_ret(0, val);
            cc.movsd(Mem(s.stackBase, stackOff * 8), val);
            recordXmmHint(s, stackOff, val);
        }
        else
        {
            InvokeNode* inv;
            cc.invoke(Out(inv), (uint64_t)jit_unbox_int,
                      FuncSignature::build<int64_t, const value::Value*>());
            inv->set_arg(0, srcAddr);
            Gp val = cc.new_gp64();
            inv->set_ret(0, val);
            cc.mov(Mem(s.stackBase, stackOff * 8), val);
            recordGpHint(s, stackOff, val);
        }
    }

    void emitEnsureUnboxed(JitEmissionState& s, int stackIdx,
                           SlotType type, SlotType targetType)
    {
        if (!s.usesBoxedTypes || !isBoxedSlotType(type)) return;
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp addr = cc.new_gp64();
        cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
        emitUnbox(s, addr, stackIdx, targetType);
    }

    void emitBoxOrCopy(JitEmissionState& s, Gp destAddr,
                       int stackOff, SlotType type)
    {
        if (s.usesBoxedTypes && isBoxedSlotType(type))
        {
            auto& cc = s.cc;
            constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
            Gp srcAddr = cc.new_gp64();
            cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>(stackOff * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, srcAddr);
        }
        else
        {
            emitBox(s, destAddr, stackOff, type);
        }
    }

    SlotType popType(JitEmissionState& s)
    {
        if (s.slotTypes.empty()) return SlotType::INT;
        SlotType t = s.slotTypes.back();
        s.slotTypes.pop_back();
        return t;
    }

    SlotType topType(const JitEmissionState& s)
    {
        return s.slotTypes.empty() ? SlotType::INT : s.slotTypes.back();
    }

    void emitCleanup(JitEmissionState& s)
    {
        // MYT-211: any pending register-cached slot must hit memory before
        // we ret — JIT-frame teardown happens here, and the inlined locals
        // cleanup helper (when usesBoxedTypes) reads the locals memory range.
        // Cheap no-op in the !boxed path when the cache is already empty.
        flushAllHints(s);
        if (!s.usesBoxedTypes) return;
        auto& cc = s.cc;
        Gp bBase = cc.new_gp64();
        cc.mov(bBase, s.boxedBase);
        Gp bCount = cc.new_gp64();
        cc.mov(bCount, static_cast<int64_t>(JitEmissionState::MAX_OP_STACK));
        InvokeNode* cleanBoxed;
        cc.invoke(Out(cleanBoxed), reinterpret_cast<uint64_t>(jit_locals_cleanup),
                  FuncSignature::build<void, value::Value*, size_t>());
        cleanBoxed->set_arg(0, bBase);
        cleanBoxed->set_arg(1, bCount);
        Gp lBase = cc.new_gp64();
        cc.mov(lBase, s.localsBase);
        Gp lCount = cc.new_gp64();
        // MYT-163: clean the inline slack too. Unwritten slack slots hold
        // monostate (zero-inited by setupCompilationFrame), so destroy is a
        // cheap no-op there — but any slot written by an inlined callee must
        // be destroyed to avoid shared_ptr leaks.
        cc.mov(lCount, static_cast<int64_t>(s.localCount + JitEmissionState::INLINE_LOCALS_SLACK));
        InvokeNode* cleanLocals;
        cc.invoke(Out(cleanLocals), reinterpret_cast<uint64_t>(jit_locals_cleanup),
                  FuncSignature::build<void, value::Value*, size_t>());
        cleanLocals->set_arg(0, lBase);
        cleanLocals->set_arg(1, lCount);
    }
}
