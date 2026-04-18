#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include "ic/InlineCacheTable.hpp"
#include "ic/TypeFeedbackCollector.hpp"
#include "../optimization/InlineAnalysis.hpp"
#include <asmjit/x86.h>
#include <cassert>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    void emitBoxCallArgs(JitEmissionState& s, size_t argCount, size_t destStartSlot)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        for (size_t i = 0; i < argCount; ++i)
        {
            int stackIdx = s.stackDepth - static_cast<int>(argCount) + static_cast<int>(i);
            SlotType argType = (stackIdx >= 0 && stackIdx < static_cast<int>(s.slotTypes.size()))
                             ? s.slotTypes[stackIdx] : SlotType::INT;
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)
                                  + static_cast<int32_t>((i + destStartSlot) * valueSize)));

            if (s.usesBoxedTypes && isBoxedSlotType(argType))
            {
                Gp srcAddr = cc.new_gp64();
                cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
                InvokeNode* cpInv;
                cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                          FuncSignature::build<void, value::Value*, const value::Value*>());
                cpInv->set_arg(0, destAddr);
                cpInv->set_arg(1, srcAddr);
            }
            else
            {
                emitBox(s, destAddr, stackIdx, argType);
            }
        }
    }

    void emitPopAndDestroyArgs(JitEmissionState& s, size_t argCount)
    {
        // popType pops from top-to-bottom, so match with top-to-bottom indices
        for (size_t i = 0; i < argCount; ++i)
        {
            SlotType at = popType(s);
            if (s.usesBoxedTypes && isBoxedSlotType(at))
            {
                // i=0 is the topmost arg (at stackDepth-1), i=1 is next, etc.
                int idx = s.stackDepth - 1 - static_cast<int>(i);
                emitValueDestroy(s, idx);
            }
        }
        s.stackDepth -= static_cast<int>(argCount);
    }

    static bool emitPushStringOp(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t constIndex = static_cast<uint32_t>(instr.operands[0]);
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp pPtr = cc.new_gp64();
        cc.mov(pPtr, s.progPtr);
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(constIndex));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_push_string),
                  FuncSignature::build<void, value::Value*,
                      const vm::bytecode::BytecodeProgram*, uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, pPtr);
        inv->set_arg(2, idx);
        s.slotTypes.push_back(SlotType::STRING);
        s.stackDepth++;
        return true;
    }

    static bool emitGetFieldOp(JitEmissionState& s,
                                const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t fieldNameIndex = static_cast<uint32_t>(instr.operands[0]);
        Gp objAddr = cc.new_gp64();
        cc.lea(objAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(fieldNameIndex));
        Gp flagsReg = cc.new_gp64();
        cc.mov(flagsReg, static_cast<int64_t>(instr.flags));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_get_field_ic),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      JitContext*, size_t, uint32_t, uint8_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, objAddr);
        inv->set_arg(2, s.ctxPtr);
        inv->set_arg(3, ipReg);
        inv->set_arg(4, idx);
        inv->set_arg(5, flagsReg);
        popType(s);
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    static bool emitSetFieldOp(JitEmissionState& s,
                                const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t fieldNameIndex = static_cast<uint32_t>(instr.operands[0]);
        SlotType valType = popType(s);
        popType(s);
        Gp newValAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, valType);
        Gp objAddr = cc.new_gp64();
        cc.lea(objAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(fieldNameIndex));
        Gp flagsReg = cc.new_gp64();
        cc.mov(flagsReg, static_cast<int64_t>(instr.flags));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_field_ic),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      const value::Value*,
                      JitContext*, size_t, uint32_t, uint8_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, objAddr);
        inv->set_arg(2, newValAddr);
        inv->set_arg(3, s.ctxPtr);
        inv->set_arg(4, ipReg);
        inv->set_arg(5, idx);
        inv->set_arg(6, flagsReg);
        if (isBoxedSlotType(valType))
            emitValueDestroy(s, s.stackDepth - 1);
        s.stackDepth--;
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    static bool emitCallStaticOp(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t nameIndex = static_cast<uint32_t>(instr.operands[0]);
        size_t argCount = instr.operands[1];
        if (argCount > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }
        emitBoxCallArgs(s, argCount);
        emitPopAndDestroyArgs(s, argCount);
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCount));
        InvokeNode* callInv;
        cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_function),
                  FuncSignature::build<void, JitContext*, uint32_t, size_t>());
        callInv->set_arg(0, s.ctxPtr);
        callInv->set_arg(1, niReg);
        callInv->set_arg(2, acReg);
        emitReturnValueCopyBoxed(s);
        return true;
    }

    // Captures the JIT emitter state that inline emission mutates while walking
    // the callee body. Used by emitInlinedMethodCallMono / emitInlinedMethodCallPoly
    // to rewind before emitting the fallback slow path.
    struct InlineEmitStateSnapshot
    {
        int stackDepth;
        std::vector<SlotType> slotTypes;
        std::unordered_map<size_t, SlotType> localTypes;
        std::unordered_map<int, JitEmissionState::CachedArrayInfo> arrayInfoCache;
        // MYT-168: currentIP is baked into the IC lookup key emitted by
        // emitCallMethodOpGeneric; the callee emission loop overwrites it per
        // instruction, so it MUST round-trip through the snapshot.
        size_t currentIP;
    };

    static InlineEmitStateSnapshot snapshotEmitStateForInline(const JitEmissionState& s)
    {
        return { s.stackDepth, s.slotTypes, s.localTypes, s.arrayInfoCache, s.currentIP };
    }

    static void restoreEmitStateForInline(JitEmissionState& s,
                                          const InlineEmitStateSnapshot& snap)
    {
        s.stackDepth     = snap.stackDepth;
        s.slotTypes      = snap.slotTypes;
        s.localTypes     = snap.localTypes;
        s.arrayInfoCache = snap.arrayInfoCache;
        s.currentIP      = snap.currentIP;
    }

    // Original (non-inlined) CALL_METHOD emission. Used as the slow path of
    // tryEmitInlinedMethodCall when the shape guard misses, and as the direct
    // emission when the site is not eligible for inlining.
    //
    // Marshals receiver + args into ctx->callArgs[], cleans the caller's boxed
    // operand stack, invokes jit_call_method_ic (which handles IC lookup and
    // either direct JIT→JIT dispatch or the interpreter path), then copies the
    // return value back onto the operand stack.
    static void emitCallMethodOpGeneric(JitEmissionState& s,
                                         const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t methodNameIndex = static_cast<uint32_t>(instr.operands[0]);
        size_t argCount = instr.operands[1];
        {
            int objIdx = s.stackDepth - static_cast<int>(argCount) - 1;
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
            Gp srcAddr = cc.new_gp64();
            cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>(objIdx * valueSize)));
            InvokeNode* cpInv;
            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            cpInv->set_arg(0, destAddr);
            cpInv->set_arg(1, srcAddr);
        }
        emitBoxCallArgs(s, argCount, 1);
        for (size_t i = 0; i < argCount; ++i)
        {
            SlotType at = popType(s);
            if (isBoxedSlotType(at))
                emitValueDestroy(s, s.stackDepth - static_cast<int>(argCount) + static_cast<int>(i));
        }
        popType(s);
        emitValueDestroy(s, s.stackDepth - static_cast<int>(argCount) - 1);
        s.stackDepth -= static_cast<int>(argCount) + 1;
        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
        Gp miReg = cc.new_gp64();
        cc.mov(miReg, static_cast<int64_t>(methodNameIndex));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCount));
        InvokeNode* callInv;
        cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_method_ic),
                  FuncSignature::build<void, JitContext*, size_t, uint32_t, size_t>());
        callInv->set_arg(0, s.ctxPtr);
        callInv->set_arg(1, ipReg);
        callInv->set_arg(2, miReg);
        callInv->set_arg(3, acReg);
        emitReturnValueCopyBoxed(s);
    }

    // MYT-163 (Phase F-a) / MYT-164 (F-b) — speculative bytecode-level
    // inlining, MONOMORPHIC IC state.
    //
    // When the method-IC at s.currentIP is MONOMORPHIC and the cached callee
    // passes InlineAnalysis eligibility, emit:
    //   * a receiver-shape guard against the cached ClassDefinition*
    //   * a copy of receiver + args into a reserved inline-locals window
    //   * the callee's bytecode ops inline, with LOAD/STORE_LOCAL remapped
    //     through s.inlineLocalsBase (see emitLoadLocal / emitStoreLocal)
    //   * a RETURN_VALUE handler that jumps to a join label leaving the result
    //     in the caller's operand-stack slot
    // On shape-guard miss: fall through to emitCallMethodOpGeneric (identical
    // to the non-inlined path), producing the same observable outcome.
    //
    // Body unchanged from F-a/F-b to keep emitted code byte-identical for
    // MONO sites (existing benchmarks must not regress). POLY state is handled
    // by emitInlinedMethodCallPoly.
    static bool emitInlinedMethodCallMono(JitEmissionState& s,
                                           const bytecode::BytecodeProgram::Instruction& instr,
                                           const ic::MethodInlineCache& cache)
    {
        const auto& entry = cache.entries[0];
        const auto* callee = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(
            entry.funcMetadata);
        const size_t argCount = instr.operands[1];

        // Sanity: arg count must match the callee's parameter count (including
        // the implicit `this`). If not, fall through to the generic path —
        // don't fabricate a match.
        if (callee->parameterCount != argCount + 1) return false;

        // MYT-164: locals base stacks for nested inlining. At depth 0 we start
        // past the caller's own locals; at depth 1 we start past the outer
        // inline frame's locals window. Bail cleanly if the cumulative window
        // would overflow INLINE_LOCALS_SLACK — better to emit the generic
        // path than corrupt unrelated frame state.
        const size_t localsBaseSlot = s.inlineStack.empty()
            ? s.localCount
            : s.inlineStack.back().localsBaseSlot
              + s.inlineStack.back().calleeMeta->localCount;
        if (localsBaseSlot + callee->localCount
            > s.localCount + JitEmissionState::INLINE_LOCALS_SLACK)
            return false;

        auto& cc = s.cc;
        asmjit::Label slowLabel = cc.new_label();
        asmjit::Label endLabel  = cc.new_label();

        // MYT-164: pre-scan the callee's bytecode range for JUMP-family
        // targets. createJumpLabels allocates one asmjit label per unique
        // target IP; the inline codegen loop binds them and jump emitters
        // (JitCompiler_ControlFlow.cpp) resolve through the top inline
        // frame's localJumpLabels before touching the outer s.labels.
        auto localJumpLabels = createJumpLabels(
            cc, s.program, callee->startOffset,
            callee->startOffset + callee->instructionCount);

        // Snapshot emitter state so the slow-path emission can rewind after
        // the fast path drifts s.stackDepth / slotTypes / currentIP during
        // callee body emission.
        const InlineEmitStateSnapshot snap = snapshotEmitStateForInline(s);

        const int receiverStackIdx = snap.stackDepth - static_cast<int>(argCount) - 1;

        // --- Fast path: shape guard + inline body ---
        emitInlineShapeGuard(s, receiverStackIdx, entry.shape, slowLabel);

        // Copy receiver + args into the callee's locals window. Primitive
        // params are unbox/rebox'd so LOAD_LOCAL in the callee body uses the
        // fast unbox-only path (matching emitArgumentUnboxing's convention
        // for top-level JIT compilation).
        emitInlineLocalCopy(s, receiverStackIdx, localsBaseSlot, *callee);

        // NB: no explicit emitValueDestroy of the caller's operand-stack slots
        // is needed here. The first write to each slot after this point — either
        // the inline body pushing onto the operand stack, the next iteration's
        // CALL_METHOD copying fresh args in, or emitCleanup at function exit —
        // goes through Value's operator=, which destructs the prior contents.
        // Skipping ~80 cycles per inline call versus the initial F-a draft.

        // Enter callee emission scope.
        InlineFrame frame;
        frame.calleeMeta = callee;
        frame.localsBaseSlot = localsBaseSlot;
        frame.inlineEndLabel = endLabel;
        frame.returnResultStackIdx = receiverStackIdx;
        frame.localJumpLabels = std::move(localJumpLabels);

        const size_t prevInlineBase = s.inlineLocalsBase;
        s.inlineStack.push_back(frame);
        s.inlineLocalsBase = localsBaseSlot;

        // Pop argCount+1 entries from slotTypes and align stackDepth so the
        // callee starts with an empty logical operand stack positioned where
        // the caller's receiver used to be.
        s.slotTypes.resize(s.slotTypes.size() - (argCount + 1));
        s.stackDepth = receiverStackIdx;
        s.arrayInfoCache.clear();

        // emitInlineLocalCopy has already recorded per-param SlotType in
        // s.localTypes — primitive params are INT/FLOAT/BOOL (fast path),
        // the receiver and any non-primitive param are BOXED.

        // onExit handler: when RETURN_VALUE fires in the callee, emitReturnValueOp
        // has already decremented s.stackDepth and popped the type. The boxed
        // result sits at s.boxedBase[s.stackDepth * valueSize], which is
        // exactly receiverStackIdx by construction — no copy needed. Just jump
        // to the join label. RETURN (no value) writes monostate likewise.
        ExitHandler onExit = [endLabel](JitEmissionState& es, size_t /*target*/) {
            es.cc.jmp(endLabel);
        };

        // Emit the callee's bytecode. MYT-164: internal jumps are permitted;
        // bind per-IP labels at the top of each iteration so JUMP/JUMP_IF_*/
        // JUMP_BACK targets inside the callee resolve through the inline
        // frame's localJumpLabels. RETURN_VALUE / RETURN still trip onExit
        // and jump to endLabel, so multiple return paths all converge there.
        // Iterate the full range — trailing unreachable ops emit dead asmjit
        // instructions, which the outer codegen loop already tolerates.
        for (size_t ip = callee->startOffset;
             ip < callee->startOffset + callee->instructionCount && !s.compileFailed;
             ++ip)
        {
            // Bind a callee-local label at this IP if the pre-scan registered
            // one. At a join point the inline operand stack is empty by
            // bytecode-compiler invariant (ControlFlowCompiler emits labels
            // at statement boundaries, never mid-expression), so reset
            // stackDepth / slotTypes to match the expected shape of the
            // next basic block. Conservative arrayInfoCache reset mirrors
            // emitCodegenLoop in JitCompiler_Core.cpp.
            auto& topFrame = s.inlineStack.back();
            auto lit = topFrame.localJumpLabels.find(ip);
            if (lit != topFrame.localJumpLabels.end())
            {
                cc.bind(lit->second);
                s.stackDepth = receiverStackIdx;
                s.slotTypes.resize(static_cast<size_t>(receiverStackIdx));
                s.arrayInfoCache.clear();
            }

            const auto& cinstr = s.program.getInstruction(ip);
            s.currentIP = ip;
            if (cinstr.opcode == OpCode::PROFILE_ENTER)
                continue;  // no-op inside an inline frame

            bool handled = false;
            if (emitCoreOps(s, cinstr)) handled = true;
            else if (emitArithmeticOps(s, cinstr)) handled = true;
            else if (emitControlFlowOps(s, cinstr, onExit)) handled = true;
            else { emitObjectOps(s, cinstr); handled = true; }
            (void)handled;
        }

        // Exit callee scope (regardless of compileFailed — we still need
        // matching state for the slow path).
        s.inlineStack.pop_back();
        s.inlineLocalsBase = prevInlineBase;

        if (s.compileFailed) return true;  // caller short-circuits; function aborts

        // Jump over the slow path to the join label.
        cc.jmp(endLabel);

        // --- Slow path: generic call when the shape guard misses ---
        cc.bind(slowLabel);

        // Restore emitter state so emitCallMethodOpGeneric sees the pre-call
        // operand stack configuration. MYT-168: this restore MUST include
        // s.currentIP — jit_call_method_ic uses it as the IC lookup key.
        restoreEmitStateForInline(s, snap);

        emitCallMethodOpGeneric(s, instr);

        // --- Join ---
        cc.bind(endLabel);
        s.arrayInfoCache.clear();  // conservative: inlined body may have touched fields
        return true;
    }

    // MYT-165 (Phase F-c) — speculative bytecode-level inlining, POLYMORPHIC
    // IC state. Emits a chain of shape guards against the N (≤4) cached
    // ClassDefinition pointers. Each guard's fall-through enters that shape's
    // inlined body; mismatch jumps to the next shape's guard (or, for the
    // last entry, to the slow-path helper call). Only one shape body
    // executes per call, so the callees share a single locals window sized
    // to the max localCount — not the sum.
    //
    // Layout of emitted code:
    //   extract classDef (once)
    //   cmp classDef, shape0 ; jne next_1
    //   [body 0] ; jmp endLabel
    //   next_1: cmp classDef, shape1 ; jne next_2
    //   [body 1] ; jmp endLabel
    //   ...
    //   next_{N-1}: cmp classDef, shape{N-1} ; jne slowLabel
    //   [body N-1] ; jmp endLabel
    //   slowLabel: call jit_call_method_ic
    //   endLabel:
    static bool emitInlinedMethodCallPoly(JitEmissionState& s,
                                           const bytecode::BytecodeProgram::Instruction& instr,
                                           const ic::MethodInlineCache& cache)
    {
        const size_t argCount = instr.operands[1];
        const uint8_t entryCount = cache.entryCount;

        // Per-entry argcount sanity + max-localCount computation. Only one
        // shape body runs per call, so the shared locals window is sized
        // to the largest callee (MAX, not SUM).
        size_t maxLocalCount = 0;
        for (uint8_t i = 0; i < entryCount; ++i)
        {
            const auto* callee = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(
                cache.entries[i].funcMetadata);
            if (callee->parameterCount != argCount + 1) return false;
            if (callee->localCount > maxLocalCount)
                maxLocalCount = callee->localCount;
        }

        const size_t localsBaseSlot = s.inlineStack.empty()
            ? s.localCount
            : s.inlineStack.back().localsBaseSlot
              + s.inlineStack.back().calleeMeta->localCount;
        if (localsBaseSlot + maxLocalCount
            > s.localCount + JitEmissionState::INLINE_LOCALS_SLACK)
            return false;

#ifndef NDEBUG
        // MethodInlineCache::addEntry already dedupes by shape; defensive check
        // that the guard chain can't silently alias two bodies.
        for (uint8_t i = 0; i < entryCount; ++i)
            for (uint8_t j = static_cast<uint8_t>(i + 1); j < entryCount; ++j)
                assert(cache.entries[i].shape != cache.entries[j].shape
                       && "POLY IC has duplicate ClassDefinition*");
#endif

        auto& cc = s.cc;

        // Pre-scan each callee's jump targets independently. F-b's
        // createJumpLabels already handles the per-entry range; we just
        // call it entryCount times.
        std::vector<std::unordered_map<size_t, asmjit::Label>> perEntryJumpLabels(entryCount);
        for (uint8_t i = 0; i < entryCount; ++i)
        {
            const auto* callee = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(
                cache.entries[i].funcMetadata);
            perEntryJumpLabels[i] = createJumpLabels(
                cc, s.program, callee->startOffset,
                callee->startOffset + callee->instructionCount);
        }

        // Snapshot emitter state. Restored before each shape body AND before
        // the slow path — each shape emission must see the same pre-call
        // configuration. The snapshot includes s.currentIP because the callee
        // emit loop overwrites it per instruction, and the slow path's
        // emitCallMethodOpGeneric bakes s.currentIP into the asmjit-emitted
        // call to jit_call_method_ic as the IC lookup key (see MYT-168).
        const InlineEmitStateSnapshot snap = snapshotEmitStateForInline(s);

        const int receiverStackIdx = snap.stackDepth - static_cast<int>(argCount) - 1;

        // Pre-allocate the guard-chain next-check labels (one per non-last
        // entry) plus the shared slow and end labels.
        std::vector<asmjit::Label> nextCheckLabels;
        nextCheckLabels.reserve(entryCount > 0 ? static_cast<size_t>(entryCount - 1) : 0);
        for (uint8_t i = 0; i + 1 < entryCount; ++i)
            nextCheckLabels.push_back(cc.new_label());
        asmjit::Label slowLabel = cc.new_label();
        asmjit::Label endLabel  = cc.new_label();

        // Extract classDef once; reuse across all guard compares.
        Gp classDefReg = emitExtractReceiverClassDef(s, receiverStackIdx);

        ExitHandler onExit = [endLabel](JitEmissionState& es, size_t /*target*/) {
            es.cc.jmp(endLabel);
        };

        for (uint8_t i = 0; i < entryCount; ++i)
        {
            // Bind this shape's check label (except shape 0, which falls
            // through from the initial cmp above — see the guard emission
            // below; shape 0's check is emitted before entering this body).
            if (i > 0)
                cc.bind(nextCheckLabels[i - 1]);

            const auto& entry = cache.entries[i];
            const auto* callee = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(
                entry.funcMetadata);

            // Guard: cmp classDef, entry.shape ; jne <next-check-or-slow>.
            const asmjit::Label missLabel = (i + 1 < entryCount)
                ? nextCheckLabels[i]
                : slowLabel;
            emitInlineShapeGuardReusingClassDef(s, classDefReg, entry.shape, missLabel);

            // Fall-through is this shape's inlined body. Each body is a
            // fresh emission — restore the snapshot before emitting so
            // stackDepth / slotTypes / localTypes are at the pre-call
            // configuration. (Body i-1 drifted these during its emission.)
            restoreEmitStateForInline(s, snap);

            emitInlineLocalCopy(s, receiverStackIdx, localsBaseSlot, *callee);

            InlineFrame frame;
            frame.calleeMeta = callee;
            frame.localsBaseSlot = localsBaseSlot;
            frame.inlineEndLabel = endLabel;
            frame.returnResultStackIdx = receiverStackIdx;
            frame.localJumpLabels = std::move(perEntryJumpLabels[i]);

            const size_t prevInlineBase = s.inlineLocalsBase;
            s.inlineStack.push_back(frame);
            s.inlineLocalsBase = localsBaseSlot;

            s.slotTypes.resize(s.slotTypes.size() - (argCount + 1));
            s.stackDepth = receiverStackIdx;
            s.arrayInfoCache.clear();

            for (size_t ip = callee->startOffset;
                 ip < callee->startOffset + callee->instructionCount && !s.compileFailed;
                 ++ip)
            {
                auto& topFrame = s.inlineStack.back();
                auto lit = topFrame.localJumpLabels.find(ip);
                if (lit != topFrame.localJumpLabels.end())
                {
                    cc.bind(lit->second);
                    s.stackDepth = receiverStackIdx;
                    s.slotTypes.resize(static_cast<size_t>(receiverStackIdx));
                    s.arrayInfoCache.clear();
                }

                const auto& cinstr = s.program.getInstruction(ip);
                s.currentIP = ip;
                if (cinstr.opcode == OpCode::PROFILE_ENTER)
                    continue;

                bool handled = false;
                if (emitCoreOps(s, cinstr)) handled = true;
                else if (emitArithmeticOps(s, cinstr)) handled = true;
                else if (emitControlFlowOps(s, cinstr, onExit)) handled = true;
                else { emitObjectOps(s, cinstr); handled = true; }
                (void)handled;
            }

            s.inlineStack.pop_back();
            s.inlineLocalsBase = prevInlineBase;

            if (s.compileFailed) return true;

            cc.jmp(endLabel);
        }

        // --- Slow path: all guards missed, route through jit_call_method_ic
        // which will handle MEGA promotion / IC maintenance as normal.
        cc.bind(slowLabel);
        restoreEmitStateForInline(s, snap);

        emitCallMethodOpGeneric(s, instr);

        cc.bind(endLabel);
        s.arrayInfoCache.clear();
        return true;
    }

    // Dispatcher: checks IC presence + eligibility + boxed mode, then routes
    // to the MONO or POLY emitter based on cache.state.
    //
    // Returns true iff inlined emission was produced (both fast + slow paths).
    // Returns false to let the caller (emitCallMethodOp) emit the generic
    // path normally — either the site is ineligible or the IC is cold.
    static bool tryEmitInlinedMethodCall(JitEmissionState& s,
                                          const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (!s.typeFeedback) return false;
        auto& icTable = s.typeFeedback->getICTable();
        if (!icTable.hasMethodIC(s.currentIP)) return false;

        auto& cache = icTable.getMethodIC(s.currentIP);
        auto decision = optimization::checkInlineEligibility(
            s.program, cache, s.currentCompilingFn, s.inlineStack.size());
        if (decision != optimization::InlineDecision::INLINE) return false;

        // Boxed-mode callee only: inlining assumes the caller emits in boxed
        // mode (guaranteed by scanOpcodesForBoxedTypes tripping on CALL_METHOD).
        // Bail if we somehow got here in unboxed mode.
        if (!s.usesBoxedTypes) return false;

        if (cache.state == ic::ICState::MONOMORPHIC)
            return emitInlinedMethodCallMono(s, instr, cache);
        if (cache.state == ic::ICState::POLYMORPHIC)
            return emitInlinedMethodCallPoly(s, instr, cache);
        return false;
    }

    static bool emitCallMethodOp(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        size_t argCount = instr.operands[1];
        if (argCount + 1 > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }
        // MYT-163: attempt speculative inlining. On success the helper emits
        // both the shape-guarded fast path and the slow-path fallback. On
        // ineligible sites (cold IC, polymorphic, ValueObject receiver, etc.)
        // it returns false and we emit the generic path unchanged.
        if (tryEmitInlinedMethodCall(s, instr))
            return true;
        emitCallMethodOpGeneric(s, instr);
        return true;
    }

    // MYT-147: iterator protocol emitters. Shape A - each opcode routes to a
    // dedicated runtime helper that calls vm->callMethodFromJit with a fixed
    // method name. Receiver is marshalled into ctx->callArgs[0] before calling.
    //
    // CRITICAL - peek vs pop semantics:
    //   GET_ITERATOR   pops receiver, pushes iterator. Net: replace receiver.
    //   HAS_NEXT/NEXT  PEEK receiver (do NOT decrement stackDepth or popType).
    //                  Copy into callArgs[0] fresh each call (a stale pointer
    //                  into boxedBase would double-free when the slot is
    //                  overwritten). Push bool / element on top of the peeked
    //                  iterator. Matches ObjectExecutor::handleIteratorHasNext
    //                  and handleIteratorNext at ObjectExecutor.cpp:976, 999.
    //   CLOSE          pops receiver, no push. Helper swallows exceptions per
    //                  ObjectExecutor.cpp:1040-1048.

    static void emitCopyReceiverToCallArgs(JitEmissionState& s, int receiverStackIdx)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp destAddr = cc.new_gp64();
        cc.lea(destAddr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
        Gp srcAddr = cc.new_gp64();
        cc.lea(srcAddr, Mem(s.boxedBase, static_cast<int32_t>(receiverStackIdx * valueSize)));
        InvokeNode* cpInv;
        cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                  FuncSignature::build<void, value::Value*, const value::Value*>());
        cpInv->set_arg(0, destAddr);
        cpInv->set_arg(1, srcAddr);
    }

    static void emitDestroyCallArg0(JitEmissionState& s)
    {
        auto& cc = s.cc;
        Gp addr = cc.new_gp64();
        cc.lea(addr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_destroy),
                  FuncSignature::build<void, value::Value*>());
        inv->set_arg(0, addr);
    }

    static bool emitGetIteratorOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);
        // Destroy the original receiver slot, pop its type. Result slot will
        // reuse the same position (net stackDepth change is 0).
        emitValueDestroy(s, receiverIdx);
        popType(s);
        s.stackDepth--;

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_iterator_get),
                  FuncSignature::build<void, value::Value*, JitContext*>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);

        emitDestroyCallArg0(s);

        s.slotTypes.push_back(SlotType::BOXED);
        s.stackDepth++;
        return true;
    }

    static bool emitIteratorHasNextOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        // MYT-156: POP receiver (mirrors handleIteratorHasNext executor change
        // and matches emitGetIteratorOp). Result slot reuses the receiver's
        // position so net stackDepth change is 0.
        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);
        emitValueDestroy(s, receiverIdx);
        popType(s);
        s.stackDepth--;

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_iterator_has_next),
                  FuncSignature::build<void, value::Value*, JitContext*>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);

        // Unbox the bool into stackBase so JUMP_IF_FALSE / JUMP_IF_TRUE can
        // consume it from the primitive stack (matches GET_FIELD / ARRAY_GET
        // mirror-to-stackBase pattern).
        Gp unboxAddr = cc.new_gp64();
        cc.lea(unboxAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* unbox;
        cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                  FuncSignature::build<int64_t, const value::Value*>());
        unbox->set_arg(0, unboxAddr);
        Gp unboxed = cc.new_gp64();
        unbox->set_ret(0, unboxed);
        cc.mov(Mem(s.stackBase, s.stackDepth * 8), unboxed);

        emitDestroyCallArg0(s);

        // Destroy the boxed mirror now that we've extracted the bool primitive.
        emitValueDestroy(s, s.stackDepth);

        s.slotTypes.push_back(SlotType::BOOL);
        s.stackDepth++;
        return true;
    }

    static bool emitIteratorNextOp(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        // MYT-156: POP receiver (mirrors handleIteratorNext executor change).
        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);
        emitValueDestroy(s, receiverIdx);
        popType(s);
        s.stackDepth--;

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_iterator_next),
                  FuncSignature::build<void, value::Value*, JitContext*>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);

        emitDestroyCallArg0(s);

        s.slotTypes.push_back(SlotType::BOXED);
        s.stackDepth++;
        return true;
    }

    static bool emitIteratorCloseOp(JitEmissionState& s)
    {
        auto& cc = s.cc;

        int receiverIdx = s.stackDepth - 1;
        emitCopyReceiverToCallArgs(s, receiverIdx);
        emitValueDestroy(s, receiverIdx);
        popType(s);
        s.stackDepth--;

        InvokeNode* inv;
        s.cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_iterator_close),
                    FuncSignature::build<void, JitContext*>());
        inv->set_arg(0, s.ctxPtr);

        emitDestroyCallArg0(s);
        return true;
    }

    static bool emitInstanceofOp(JitEmissionState& s,
                                  const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t typeIndex = static_cast<uint32_t>(instr.operands[0]);
        SlotType vType = popType(s);
        Gp valAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, vType);
        Gp pPtr = cc.new_gp64();
        cc.mov(pPtr, s.progPtr);
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(typeIndex));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_instanceof),
                  FuncSignature::build<int64_t, const value::Value*,
                      const vm::bytecode::BytecodeProgram*, uint32_t>());
        inv->set_arg(0, valAddr);
        inv->set_arg(1, pPtr);
        inv->set_arg(2, idx);
        Gp result = cc.new_gp64();
        inv->set_ret(0, result);
        if (isBoxedSlotType(vType))
            emitValueDestroy(s, s.stackDepth - 1);
        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), result);
        s.slotTypes.push_back(SlotType::BOOL);
        return true;
    }

    static bool emitCastOp(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t typeIndex = static_cast<uint32_t>(instr.operands[0]);
        SlotType srcType = popType(s);
        Gp srcAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, srcType);
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        Gp pPtr = cc.new_gp64();
        cc.mov(pPtr, s.progPtr);
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(typeIndex));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_cast),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      const vm::bytecode::BytecodeProgram*, uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, srcAddr);
        inv->set_arg(2, pPtr);
        inv->set_arg(3, idx);
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    static bool emitNewObjectOp(JitEmissionState& s,
                                 const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t classIndex = static_cast<uint32_t>(instr.operands[0]);
        size_t argCount = instr.operands[1];
        if (argCount > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }
        emitBoxCallArgs(s, argCount);
        emitPopAndDestroyArgs(s, argCount);
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp ciReg = cc.new_gp64();
        cc.mov(ciReg, static_cast<int64_t>(classIndex));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCount));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_new_object),
                  FuncSignature::build<void, value::Value*, JitContext*,
                      uint32_t, size_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);
        inv->set_arg(2, ciReg);
        inv->set_arg(3, acReg);
        s.slotTypes.push_back(SlotType::OBJECT);
        s.stackDepth++;
        return true;
    }

    bool emitObjectOps(JitEmissionState& s,
                       const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        if (emitArrayOps(s, instr)) return true;

        switch (instr.opcode)
        {
            case OpCode::PUSH_STRING:    return emitPushStringOp(s, instr);
            case OpCode::GET_FIELD:      return emitGetFieldOp(s, instr);
            case OpCode::SET_FIELD:      return emitSetFieldOp(s, instr);
            case OpCode::CALL_STATIC:    return emitCallStaticOp(s, instr);
            case OpCode::CALL_METHOD:    return emitCallMethodOp(s, instr);
            case OpCode::INSTANCEOF:     return emitInstanceofOp(s, instr);
            case OpCode::CAST:           return emitCastOp(s, instr);
            case OpCode::NEW_OBJECT:
            case OpCode::NEW_VALUE_OBJECT: return emitNewObjectOp(s, instr);

            case OpCode::GET_ITERATOR:       return emitGetIteratorOp(s);
            case OpCode::ITERATOR_HAS_NEXT:  return emitIteratorHasNextOp(s);
            case OpCode::ITERATOR_NEXT:      return emitIteratorNextOp(s);
            case OpCode::ITERATOR_CLOSE:     return emitIteratorCloseOp(s);

            case OpCode::OBJECT_TO_VALUE:
            {
                Gp addr = cc.new_gp64();
                cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_object_to_value),
                          FuncSignature::build<void, value::Value*>());
                inv->set_arg(0, addr);
                popType(s);
                s.slotTypes.push_back(SlotType::VALUE_OBJECT);
                return true;
            }

            default:
                return false;
        }
    }
}
