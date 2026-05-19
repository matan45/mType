#include "JitEmissionState.hpp"
#include "JitCompiler_Objects.hpp"
#include "JitCompiler.hpp"
#include "JitCodeCache.hpp"
#include "JitHelpers.hpp"
#include "ic/InlineCacheTable.hpp"
#include "ic/TypeFeedbackCollector.hpp"
#include "../optimization/InlineAnalysis.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include <asmjit/x86.h>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;

    bool emitCallStaticOp(JitEmissionState& s,
                          const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t nameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];
        if (argCount > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }
        if (tryEmitInlinedStaticCall(s, instr))
            return true;
        emitBoxCallArgs(s, argCount);
        emitPopAndDestroyArgs(s, argCount);
        // Pass the call-site IP so jit_call_function_ic can cache the resolved
        // FunctionMetadata. Static call sites are monomorphic by definition;
        // the cache eliminates the per-call program->getFunction hashmap probe.
        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCount));
        InvokeNode* callInv;
        cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_function_ic),
                  FuncSignature::build<void, JitContext*, size_t, uint32_t, size_t>());
        callInv->set_arg(0, s.ctxPtr);
        callInv->set_arg(1, ipReg);
        callInv->set_arg(2, niReg);
        callInv->set_arg(3, acReg);
        emitReturnValueCopyBoxed(s);
        return true;
    }

    // Original (non-inlined) CALL_METHOD emission. Used as the slow path of
    // tryEmitInlinedMethodCall when the shape guard misses, and as the direct
    // emission when the site is not eligible for inlining.
    //
    // Marshals receiver + args into ctx->callArgs[], cleans the caller's
    // boxed operand stack, invokes jit_call_method_ic (which handles IC
    // lookup and either direct JIT→JIT dispatch or the interpreter path),
    // then copies the return value back onto the operand stack.
    void emitCallMethodOpGeneric(JitEmissionState& s,
                                 const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t methodNameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];
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

    // Speculative bytecode-level inlining, MONOMORPHIC IC state.
    //
    // When the method-IC at s.currentIP is MONOMORPHIC and the cached callee
    // passes InlineAnalysis eligibility, emit a receiver-shape guard, copy of
    // receiver+args into a reserved inline-locals window, the callee's
    // bytecode ops inline (LOAD/STORE_LOCAL remapped through
    // s.inlineLocalsBase), and a RETURN_VALUE handler that jumps to a join
    // label leaving the result in the caller's operand-stack slot. On
    // shape-guard miss: fall through to emitCallMethodOpGeneric.
    static bool emitInlinedMethodCallMono(JitEmissionState& s,
                                          const bytecode::BytecodeProgram::Instruction& instr,
                                          const ic::MethodInlineCache& cache,
                                          optimization::InlineDecision entryDecision)
    {
        const auto& entry = cache.entries[0];
        // Value-class write methods inline with a materialised temp
        // ObjectInstance in local-0. Eligibility analyzer flags this via
        // INLINE_VALUE_REQUIRES_MATERIALISATION; read-only value-class
        // methods stay on the cheap raw-memcpy path.
        const bool materialiseValueReceiver =
            entryDecision == optimization::InlineDecision::INLINE_VALUE_REQUIRES_MATERIALISATION;
        const auto* callee = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(
            entry.funcMetadata);
        const size_t argCount = instr.inlineOperands[1];

        // Arg count must match the callee's parameter count (including the
        // implicit `this`). Otherwise fall through to the generic path.
        if (callee->parameterCount != argCount + 1) return false;

        // Locals base stacks for nested inlining. At depth 0 we start past
        // the caller's own locals; at depth 1 we start past the outer inline
        // frame's locals window. Bail cleanly if the cumulative window would
        // overflow INLINE_LOCALS_SLACK.
        const size_t localsBaseSlot = s.inlineStack.empty()
            ? s.localCount
            : s.inlineStack.back().localsBaseSlot
              + s.inlineStack.back().calleeMeta->localCount;
        if (localsBaseSlot + callee->localCount
            > s.localCount + JitEmissionState::INLINE_LOCALS_SLACK)
            return false;

        // Peak-operand-stack guard. Reject candidates whose caller_depth +
        // callee_peak would exceed MAX_OP_STACK and overrun cc.new_stack
        // (trips /GS-cookie fastfail).
        {
            const size_t calleePeak = computeCalleePeakOperandStack(s.program, *callee);
            if (static_cast<size_t>(s.stackDepth) + calleePeak
                > JitEmissionState::MAX_OP_STACK)
                return false;
        }

        auto& cc = s.cc;
        asmjit::Label slowLabel = cc.new_label();
        asmjit::Label endLabel  = cc.new_label();

        // Pre-scan callee's bytecode range for JUMP-family targets.
        // createJumpLabels allocates one asmjit label per unique target IP;
        // the inline codegen loop binds them and jump emitters resolve through
        // the top inline frame's localJumpLabels before touching s.labels.
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
        // fast unbox-only path.
        emitInlineLocalCopy(s, receiverStackIdx, localsBaseSlot, *callee,
                            materialiseValueReceiver);

        // No explicit emitValueDestroy of caller's operand-stack slots is
        // needed: the first write to each slot after this point — inline body
        // pushing onto the operand stack, the next iteration's CALL_METHOD
        // copying fresh args in, or emitCleanup at function exit — goes
        // through Value's operator=, which destructs the prior contents.

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

        // onExit handler: when RETURN_VALUE fires in the callee,
        // emitReturnValueOp has already decremented s.stackDepth and popped
        // the type. The boxed result sits at s.boxedBase[receiverStackIdx],
        // matching the slow path's boxed write via emitReturnValueCopyBoxed.
        //
        // The slow path also mirrors the boxed value to s.stackBase via
        // jit_unbox_int. The fast path must do the same so downstream
        // boxed-mode consumers (ADD_INT, outer RETURN_VALUE, CONCAT_STRING,
        // etc.) read consistent physical state at endLabel.
        ExitHandler onExit = [endLabel, receiverStackIdx, localsBaseSlot, callee]
            (JitEmissionState& es, size_t /*target*/) {
            emitInlineReturnMaterialize(es, receiverStackIdx, es.lastReturnSlotType);
            emitInlineLocalDestroy(es, localsBaseSlot, *callee);
            // Pop the inlined-callee class pushed at body entry.
            {
                InvokeNode* pop = nullptr;
                es.cc.invoke(Out(pop),
                          reinterpret_cast<uint64_t>(jit_pop_inlined_class),
                          FuncSignature::build<void, JitContext*>());
                pop->set_arg(0, es.ctxPtr);
            }
            es.cc.jmp(endLabel);
        };

        // Emit the callee's bytecode via the shared helper. Internal jumps
        // are permitted; the helper binds per-IP labels and resets
        // (stackDepth, slotTypes, arrayInfoCache) to the receiver-baseline at
        // each join point. RETURN_VALUE / RETURN trip onExit and jump to
        // endLabel.
        emitInlineCalleeBody(s, *callee, receiverStackIdx, onExit);

        s.inlineStack.pop_back();
        s.inlineLocalsBase = prevInlineBase;

        if (s.compileFailed) return true;

        // Trailing jmp(endLabel) as a fall-through terminator so any dead
        // bytecode emitted after the final RETURN_VALUE can't leak control
        // into the slow path's bound label below. At runtime this jmp is
        // unreachable for well-formed callees.
        cc.jmp(endLabel);

        // --- Slow path: generic call when the shape guard misses ---
        cc.bind(slowLabel);

        // Restore emitter state so emitCallMethodOpGeneric sees the pre-call
        // operand stack. The restore MUST include s.currentIP —
        // jit_call_method_ic uses it as the IC lookup key.
        restoreEmitStateForInline(s, snap);

        emitCallMethodOpGeneric(s, instr);

        cc.bind(endLabel);
        normalizeInlineReturnJoinState(s, receiverStackIdx, snap.currentIP);

        return true;
    }

    // Speculative bytecode-level inlining, POLYMORPHIC IC state. Emits a
    // chain of shape guards against the cached ClassDefinition pointers.
    // Each guard's fall-through enters that shape's inlined body; mismatch
    // jumps to the next shape's guard (or, for the last entry, to the
    // slow-path helper call). Only one shape body executes per call, so the
    // callees share a single locals window sized to the max localCount.
    //
    // Layout:
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
                                          const ic::MethodInlineCache& cache,
                                          const std::array<optimization::InlineDecision,
                                                           ic::IC_MAX_POLYMORPHIC_ENTRIES>&
                                              perEntryDecisions)
    {
        const size_t argCount = instr.inlineOperands[1];
        const uint8_t entryCount = cache.entryCount;

        // Both INLINE and INLINE_VALUE_REQUIRES_MATERIALISATION emit an inline
        // body. The materialise variant additionally injects a value-class
        // temp into local-0 before the body runs.
        auto isInlineableArm = [](optimization::InlineDecision d) {
            return d == optimization::InlineDecision::INLINE ||
                   d == optimization::InlineDecision::INLINE_VALUE_REQUIRES_MATERIALISATION;
        };

        // Per-entry argcount sanity + max-localCount computation. Only one
        // shape body runs per call, so the shared locals window is sized to
        // the largest callee (MAX, not SUM). Non-inlineable entries route
        // through emitCallMethodOpGeneric per-shape and never paste their
        // callee body — skip their localCount / operand-stack-peak
        // contribution so a single oversized helper-routed shape doesn't
        // inflate the shared inline window.
        size_t maxLocalCount = 0;
        size_t maxCalleePeak = 0;
        for (uint8_t i = 0; i < entryCount; ++i)
        {
            const auto* callee = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(
                cache.entries[i].funcMetadata);
            if (callee->parameterCount != argCount + 1) return false;
            if (!isInlineableArm(perEntryDecisions[i]))
                continue;
            if (callee->localCount > maxLocalCount)
                maxLocalCount = callee->localCount;
            const size_t p = computeCalleePeakOperandStack(s.program, *callee);
            if (p > maxCalleePeak) maxCalleePeak = p;
        }

        const size_t localsBaseSlot = s.inlineStack.empty()
            ? s.localCount
            : s.inlineStack.back().localsBaseSlot
              + s.inlineStack.back().calleeMeta->localCount;
        if (localsBaseSlot + maxLocalCount
            > s.localCount + JitEmissionState::INLINE_LOCALS_SLACK)
            return false;

        if (static_cast<size_t>(s.stackDepth) + maxCalleePeak
            > JitEmissionState::MAX_OP_STACK)
            return false;

#ifndef NDEBUG
        // MethodInlineCache::addEntry already dedupes by shape; defensive
        // check that the guard chain can't silently alias two bodies.
        for (uint8_t i = 0; i < entryCount; ++i)
            for (uint8_t j = static_cast<uint8_t>(i + 1); j < entryCount; ++j)
                assert(cache.entries[i].shape != cache.entries[j].shape
                       && "POLY IC has duplicate ClassDefinition*");
#endif

        auto& cc = s.cc;

        // Pre-scan each callee's jump targets independently. Skip non-inlineable
        // entries — their dispatch goes through emitCallMethodOpGeneric which
        // has its own label bookkeeping.
        std::vector<std::unordered_map<size_t, asmjit::Label>> perEntryJumpLabels(entryCount);
        for (uint8_t i = 0; i < entryCount; ++i)
        {
            if (!isInlineableArm(perEntryDecisions[i]))
                continue;
            const auto* callee = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(
                cache.entries[i].funcMetadata);
            perEntryJumpLabels[i] = createJumpLabels(
                cc, s.program, callee->startOffset,
                callee->startOffset + callee->instructionCount);
        }

        // Snapshot emitter state. Restored before each shape body AND before
        // the slow path. The snapshot includes s.currentIP because the slow
        // path's emitCallMethodOpGeneric bakes s.currentIP into the
        // asmjit-emitted call to jit_call_method_ic as the IC lookup key.
        const InlineEmitStateSnapshot snap = snapshotEmitStateForInline(s);

        const int receiverStackIdx = snap.stackDepth - static_cast<int>(argCount) - 1;

        // Pre-allocate the guard-chain next-check labels (one per non-last
        // entry) plus shared slow and end labels.
        std::vector<asmjit::Label> nextCheckLabels;
        nextCheckLabels.reserve(entryCount > 0 ? static_cast<size_t>(entryCount - 1) : 0);
        for (uint8_t i = 0; i + 1 < entryCount; ++i)
            nextCheckLabels.push_back(cc.new_label());
        asmjit::Label slowLabel = cc.new_label();
        asmjit::Label endLabel  = cc.new_label();

        // Extract classDef once; reuse across all guard compares.
        Gp classDefReg = emitExtractReceiverClassDef(s, receiverStackIdx);

        for (uint8_t i = 0; i < entryCount; ++i)
        {
            if (i > 0)
                cc.bind(nextCheckLabels[i - 1]);

            const auto& entry = cache.entries[i];
            const auto* callee = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(
                entry.funcMetadata);

            const asmjit::Label missLabel = (i + 1 < entryCount)
                ? nextCheckLabels[i]
                : slowLabel;
            emitInlineShapeGuardReusingClassDef(s, classDefReg, entry.shape, missLabel);

            // Per-entry routing. Inlineable entries paste the callee body
            // in-line. Non-inlineable entries (oversized, has TRY/CATCH, etc.)
            // keep the same shape guard chain width but route this shape's
            // matched-guard fallthrough through emitCallMethodOpGeneric or a
            // direct dispatch — same helpers the all-miss slow path uses. The
            // inlineable shapes pay no IC dispatch overhead.
            if (!isInlineableArm(perEntryDecisions[i]))
            {
                restoreEmitStateForInline(s, snap);

                // If this rejected entry has a JIT-compiled callee, direct-
                // dispatch instead of routing through jit_call_method_ic's
                // mini-interpreter hop. The runtime shape guard already
                // matched at this point.
                const void* directTarget = resolveCachedJit(s, cache.entries[i]);
                if (directTarget)
                {
                    const auto& directEntry = cache.entries[i];
                    emitMethodCallMarshalAndCleanup(s, argCount);
                    {
                        Gp cjReg = cc.new_gp64();
                        cc.mov(cjReg, reinterpret_cast<uint64_t>(directTarget));
                        Gp progReg = cc.new_gp64();
                        cc.mov(progReg, reinterpret_cast<uint64_t>(directEntry.program));
                        Gp metaReg = cc.new_gp64();
                        cc.mov(metaReg, reinterpret_cast<uint64_t>(directEntry.funcMetadata));
                        Gp acReg = cc.new_gp64();
                        cc.mov(acReg, static_cast<int64_t>(argCount + 1));
                        InvokeNode* dcInv;
                        cc.invoke(Out(dcInv),
                                  reinterpret_cast<uint64_t>(jit_call_method_direct),
                                  FuncSignature::build<void, JitContext*, const void*,
                                                       const bytecode::BytecodeProgram*,
                                                       const void*, size_t>());
                        dcInv->set_arg(0, s.ctxPtr);
                        dcInv->set_arg(1, cjReg);
                        dcInv->set_arg(2, progReg);
                        dcInv->set_arg(3, metaReg);
                        dcInv->set_arg(4, acReg);
                    }
                    emitReturnValueCopyBoxed(s);
                }
                else
                {
                    emitCallMethodOpGeneric(s, instr);
                }
                if (s.compileFailed) return true;
                cc.jmp(endLabel);
                continue;
            }

            // Fall-through is this shape's inlined body. Each body is a fresh
            // emission — restore the snapshot before emitting.
            restoreEmitStateForInline(s, snap);

            // Materialise the value-class temp only for arms whose callee
            // writes its own field(s); read-only value-class arms stay on
            // the cheap raw-memcpy donation path.
            const bool materialiseValueReceiver =
                perEntryDecisions[i] ==
                optimization::InlineDecision::INLINE_VALUE_REQUIRES_MATERIALISATION;
            emitInlineLocalCopy(s, receiverStackIdx, localsBaseSlot, *callee,
                                materialiseValueReceiver);

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

            // Per-shape onExit. Captures this iteration's callee pointer so
            // emitInlineLocalDestroy releases the correct parameter set, and
            // mirrors the boxed return to stackBase so the shared endLabel
            // join has consistent runtime state across all shapes and the
            // slow path.
            ExitHandler onExit = [endLabel, receiverStackIdx, localsBaseSlot, callee]
                (JitEmissionState& es, size_t /*target*/) {
                emitInlineReturnMaterialize(es, receiverStackIdx, es.lastReturnSlotType);
                emitInlineLocalDestroy(es, localsBaseSlot, *callee);
                {
                    InvokeNode* pop = nullptr;
                    es.cc.invoke(Out(pop),
                              reinterpret_cast<uint64_t>(jit_pop_inlined_class),
                              FuncSignature::build<void, JitContext*>());
                    pop->set_arg(0, es.ctxPtr);
                }
                es.cc.jmp(endLabel);
            };

            emitInlineCalleeBody(s, *callee, receiverStackIdx, onExit);

            s.inlineStack.pop_back();
            s.inlineLocalsBase = prevInlineBase;

            if (s.compileFailed) return true;

            // Defensive trailing jmp to prevent dead-code fall-through into
            // the next shape's guard or the slow path.
            cc.jmp(endLabel);
        }

        // --- Slow path: all guards missed, route through jit_call_method_ic
        // which will handle MEGA promotion / IC maintenance as normal.
        cc.bind(slowLabel);
        restoreEmitStateForInline(s, snap);

        emitCallMethodOpGeneric(s, instr);

        cc.bind(endLabel);
        normalizeInlineReturnJoinState(s, receiverStackIdx, snap.currentIP);

        return true;
    }

    // Dispatcher: checks IC presence + eligibility + boxed mode, then routes
    // to MONO or POLY emitter. Returns true iff inlined emission was produced
    // (both fast + slow paths). Returns false to let the caller emit the
    // generic path normally — either the site is ineligible or the IC is cold.
    static bool tryEmitInlinedMethodCall(JitEmissionState& s,
                                          const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (!s.typeFeedback) return false;
        auto& icTable = s.typeFeedback->getICTable();
        if (!icTable.hasMethodIC(s.currentIP)) return false;

        auto& cache = icTable.getMethodIC(s.currentIP);
        // Per-entry decisions. Default-init to INLINE so unused slots
        // (entryCount < IC_MAX_POLYMORPHIC_ENTRIES) and the MONO path (single
        // entry, decision implicit in the top-level return) don't carry
        // meaningful data. The POLY emitter only consults [0..entryCount).
        std::array<optimization::InlineDecision, ic::IC_MAX_POLYMORPHIC_ENTRIES>
            perEntryDecisions{};
        for (auto& d : perEntryDecisions)
            d = optimization::InlineDecision::INLINE;
        auto decision = optimization::checkInlineEligibility(
            s.program, cache, s.currentCompilingFn, s.inlineStack.size(),
            s.isOSRCompilation, &perEntryDecisions);
        // Telemetry — every site decision (including INLINE) is counted so
        // --jit-stats can report the eligibility breakdown. When a site
        // mixes inlineable and helper-routed entries, `decision` is INLINE.
        if (s.inlineDecisions) s.inlineDecisions->bumpMethod(decision);

        if (decision != optimization::InlineDecision::INLINE) return false;

        // Boxed-mode callee only: inlining assumes boxed emission. Bail if
        // we somehow got here in unboxed mode.
        if (!s.usesBoxedTypes) return false;

        if (cache.state == ic::ICState::MONOMORPHIC)
            return emitInlinedMethodCallMono(s, instr, cache, perEntryDecisions[0]);
        if (cache.state == ic::ICState::POLYMORPHIC)
            return emitInlinedMethodCallPoly(s, instr, cache, perEntryDecisions);
        return false;
    }

    // Lazy lookup of the callee's JIT pointer. The IC entry's cachedJit slot
    // is populated at IC fill time, which is BEFORE the callee tiers up. So
    // that slot is null on hot benchmarks where caller and callee tier up in
    // parallel. Falls back to a fresh JitCodeCache lookup against the entry's
    // qualified name. Returns nullptr for value-class receivers, missing
    // metadata, or no JIT entry yet.
    const void* resolveCachedJit(JitEmissionState& s,
                                  const ic::MethodICEntry& entry)
    {
        if (entry.receiverIsValueObject) return nullptr;
        const auto* funcMeta = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(
            entry.funcMetadata);
        if (!funcMeta || !entry.program) return nullptr;
        if (entry.cachedJit) return entry.cachedJit;
        if (!s.codeCache) return nullptr;
        return reinterpret_cast<const void*>(
            s.codeCache->lookup(entry.qualifiedName));
    }

    // Shared by tryEmitDirectMethodCall and the POLY inliner's rejected-arm
    // branch. Mirrors emitCallMethodOpGeneric's marshal+cleanup half without
    // the cc.invoke to jit_call_method_ic.
    void emitMethodCallMarshalAndCleanup(JitEmissionState& s, size_t argCount)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
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
    }

    static void emitMethodDirectInvoke(JitEmissionState& s,
                                        const void* cachedJit,
                                        const bytecode::BytecodeProgram* calleeProgram,
                                        const void* funcMetadata,
                                        size_t argCountPlusReceiver)
    {
        auto& cc = s.cc;
        Gp cjReg = cc.new_gp64();
        cc.mov(cjReg, reinterpret_cast<uint64_t>(cachedJit));
        Gp progReg = cc.new_gp64();
        cc.mov(progReg, reinterpret_cast<uint64_t>(calleeProgram));
        Gp metaReg = cc.new_gp64();
        cc.mov(metaReg, reinterpret_cast<uint64_t>(funcMetadata));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCountPlusReceiver));
        InvokeNode* dcInv;
        cc.invoke(Out(dcInv),
                  reinterpret_cast<uint64_t>(jit_call_method_direct),
                  FuncSignature::build<void, JitContext*, const void*,
                                       const bytecode::BytecodeProgram*,
                                       const void*, size_t>());
        dcInv->set_arg(0, s.ctxPtr);
        dcInv->set_arg(1, cjReg);
        dcInv->set_arg(2, progReg);
        dcInv->set_arg(3, metaReg);
        dcInv->set_arg(4, acReg);
    }

    static bool tryEmitDirectMethodCall(JitEmissionState& s,
                                         const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (!s.typeFeedback) return false;
        auto& icTable = s.typeFeedback->getICTable();
        if (!icTable.hasMethodIC(s.currentIP)) return false;

        auto& cache = icTable.getMethodIC(s.currentIP);
        if (cache.state != ic::ICState::MONOMORPHIC &&
            cache.state != ic::ICState::POLYMORPHIC)
            return false;

        bool anyCachedJit = false;
        for (uint8_t i = 0; i < cache.entryCount; ++i)
        {
            if (resolveCachedJit(s, cache.entries[i]))
            {
                anyCachedJit = true;
                break;
            }
        }
        if (!anyCachedJit) return false;

        const size_t argCount = instr.inlineOperands[1];
        if (argCount + 1 > JitContext::MAX_CALL_ARGS) return false;

        const int receiverStackIdx = s.stackDepth - static_cast<int>(argCount) - 1;
        if (receiverStackIdx < 0) return false;

        // Boxed-mode required: callArgs marshalling, emitReturnValueCopyBoxed,
        // and the shape-guard's boxed-receiver extract all rely on the boxed
        // operand stack.
        if (!s.usesBoxedTypes) return false;

        auto& cc = s.cc;

        asmjit::Label slowLabel = cc.new_label();
        asmjit::Label joinLabel = cc.new_label();

        const InlineEmitStateSnapshot snap = snapshotEmitStateForInline(s);

        if (cache.state == ic::ICState::MONOMORPHIC)
        {
            const auto& entry = cache.entries[0];
            const void* monoTarget = resolveCachedJit(s, entry);
            if (!monoTarget || !entry.shape)
            {
                // Defensive — the anyCachedJit gate above should exclude this.
                return false;
            }

            emitInlineShapeGuard(s, receiverStackIdx, entry.shape, slowLabel);
            emitMethodCallMarshalAndCleanup(s, argCount);
            emitMethodDirectInvoke(s, monoTarget, entry.program,
                                   entry.funcMetadata, argCount + 1);
            emitReturnValueCopyBoxed(s);
            cc.jmp(joinLabel);
        }
        else
        {
            // POLY: chained guards over up-to-4 cached shapes. Mirrors
            // emitInlinedMethodCallPoly's guard chain.
            const uint8_t entryCount = cache.entryCount;

            std::vector<asmjit::Label> nextCheckLabels;
            nextCheckLabels.reserve(entryCount > 0
                                    ? static_cast<size_t>(entryCount - 1) : 0);
            for (uint8_t i = 0; i + 1 < entryCount; ++i)
                nextCheckLabels.push_back(cc.new_label());

            Gp classDefReg = emitExtractReceiverClassDef(s, receiverStackIdx);

            for (uint8_t i = 0; i < entryCount; ++i)
            {
                if (i > 0) cc.bind(nextCheckLabels[i - 1]);

                const auto& entry = cache.entries[i];
                const asmjit::Label missLabel = (i + 1 < entryCount)
                    ? nextCheckLabels[i]
                    : slowLabel;

                const void* polyTarget = resolveCachedJit(s, entry);
                if (!polyTarget || !entry.shape)
                {
                    // Entry has no JIT'd callee yet — skip to next guard.
                    cc.jmp(missLabel);
                    continue;
                }

                emitInlineShapeGuardReusingClassDef(s, classDefReg,
                                                    entry.shape, missLabel);

                // Each shape's body runs from the pre-call snapshot. Iter 0
                // already has snap state; iterations 1+ must restore.
                if (i > 0) restoreEmitStateForInline(s, snap);

                emitMethodCallMarshalAndCleanup(s, argCount);
                emitMethodDirectInvoke(s, polyTarget, entry.program,
                                       entry.funcMetadata, argCount + 1);
                emitReturnValueCopyBoxed(s);
                cc.jmp(joinLabel);
            }
        }

        cc.bind(slowLabel);
        restoreEmitStateForInline(s, snap);
        emitCallMethodOpGeneric(s, instr);
        // emitCallMethodOpGeneric ends with its own emitReturnValueCopyBoxed,
        // so the slow path falls through to joinLabel with the operand stack
        // in the same configuration the fast path leaves it in.

        cc.bind(joinLabel);
        return true;
    }

    bool emitCallMethodOp(JitEmissionState& s,
                          const bytecode::BytecodeProgram::Instruction& instr)
    {
        size_t argCount = instr.inlineOperands[1];
        if (argCount + 1 > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }
        if (tryEmitProtocolFastMethodCall(s, instr))
            return true;
        // Speculative inlining. On success the helper emits both the
        // shape-guarded fast path and the slow-path fallback. On ineligible
        // sites it returns false and we emit the generic path.
        if (tryEmitInlinedMethodCall(s, instr))
            return true;
        // Direct JIT-to-JIT dispatch when the IC entry has a cached
        // JitFunction pointer. The shape guard + direct call skips the IC
        // mini-interpreter for the hot path. Falls through to
        // emitCallMethodOpGeneric when no entry has a cached JIT pointer
        // (callee not JIT'd yet).
        if (tryEmitDirectMethodCall(s, instr))
            return true;
        emitCallMethodOpGeneric(s, instr);
        return true;
    }
}
