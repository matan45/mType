#pragma once

#include "../bytecode/BytecodeProgram.hpp"
#include "../jit/ic/InlineCacheTypes.hpp"

#include <array>
#include <cstddef>
#include <string>

namespace vm::jit::ic {
    struct MethodICEntry;
    struct MethodInlineCache;
}

namespace vm::optimization
{
    // MYT-163 (Phase F-a) — speculative JIT inlining eligibility.
    //
    // The check is pure: given a callee's FunctionMetadata, the IC state at the
    // call site, and the caller's current inlining depth, returns one of the
    // Decision values. INLINE means every rule passed; any other value is a
    // fall-through signal that lets the emitter continue with the generic
    // jit_call_method_ic path unchanged.
    //
    // F-a restricts to straight-line MONO callees with ObjectInstance
    // receivers. F-b adds internal jumps + nested inlining. F-c (MYT-165)
    // accepts POLYMORPHIC IC state: every entry in cache.entries[0..entryCount)
    // must pass the per-entry checks AND the combined instructionCount across
    // entries must not exceed INLINE_SIZE_LIMIT * IC_MAX_POLYMORPHIC_ENTRIES
    // (= 64 ops). The emitter reads cache.state to choose MONO vs POLY
    // emission paths; the eligibility function is unchanged for callers.

    enum class InlineDecision
    {
        INLINE,
        CALLEE_TOO_BIG,
        HAS_TRY_CATCH,
        HAS_ASYNC,
        HAS_UPVALUES,
        HAS_SUPER_CALL,
        HAS_NESTED_CALL,
        HAS_INTERNAL_JUMPS,
        SELF_RECURSIVE,
        DEPTH_EXCEEDED,
        IC_NOT_MONOMORPHIC,
        UNKNOWN_SHAPE,
        VALUE_OBJECT_RECEIVER,       // MYT-167 (F-e): legacy — no longer emitted; kept for log stability
        VALUE_OBJECT_WRITES_FIELDS,  // MYT-167 (F-e): ValueObject receiver + write-containing callee
        CALLEE_NATIVE,
        CALLEE_NOT_FOUND,
        // MYT-185: emitted for callee bodies containing opcodes the JIT
        // inliner's codegen loop does not handle (e.g. STRING_BUILD). The
        // slow-path generic dispatch honours these opcodes via the interpreter
        // bailout, so rejecting inlining is safe and correct.
        HAS_UNSUPPORTED_OPCODE
    };

    // Size gate — callee bytecode instruction count. MYT-210: bumped from
    // 16 to 32 so multi-statement leaves (e.g. distanceSq with 4 args + two
    // intermediate locals + a sum-of-squares return) clear the gate. The
    // tighter 16 was set in MYT-163 for one-liner getters; plain functions
    // and non-trivial methods routinely run 17-30 ops.
    constexpr size_t INLINE_SIZE_LIMIT = 32;

    // Max nested inline depth. The caller tracks depth in JitEmissionState
    // (inlineStack.size()). F-a originally shipped depth 1; F-b raised to
    // 2 for nested-inline support.
    //
    // MYT-252: kept at 2 (the original F-b nested-inline target). Two
    // distinct bugs were on the path here:
    //
    //  (a) GET_FIELD on a primitive field in usesBoxedTypes mode wrote
    //      only to boxedBase, but downstream NOT / JUMP_IF_FALSE /
    //      JUMP_IF_TRUE / arith read raw primitives from stackBase —
    //      CALL_METHOD's slow path mirrors via emitReturnValueCopyBoxed
    //      (MYT-154); GET_FIELD had no equivalent. Fixed in
    //      emitGetFieldHelperInvoke + tryEmitInlinedFieldGet (this branch).
    //
    //  (b) A separate hang on stream_pipeline_hot triggers when both
    //      count()'s outer loop AND FilteringIterator::advance()'s inner
    //      loop become hot enough to OSR-compile (M >= ~6 elements).
    //      Once both are OSR'd, count()'s depth-2 inlined view of
    //      hasNextElement reads it as permanently true.
    //
    //      Confirmed OSR-specific (historically): with OSR disabled but
    //      inlining on (function-level JIT only), depth-2 inlined chain
    //      compiles and runs correctly. The InlineAnalysis decision
    //      sequence is identical between passing (M=4) and hanging
    //      (M=10) runs — the bug is in OSR codegen / re-entry, not
    //      in inline eligibility. Tracked as MYT-254. Cousin of
    //      MYT-248/249/250 silent-OSR family (which itself was rooted
    //      in operand-stack overrun and fixed in MYT-251).
    //
    // Workaround for (b) until MYT-254 lands: lower this constant to 0
    // or 1, which keeps depth-1 inlining but disables depth-2.
    // (The OSR/inlining MTYPE_DISABLE_* env vars referenced in earlier
    // comment revisions were removed alongside MYT-251 — the only
    // remaining knob is this constant.)
    //
    // The "InlineDepthGuard" hypothesis from earlier comment iterations
    // was unrelated and stays reverted.
    constexpr size_t INLINE_DEPTH_LIMIT = 2;

    // MYT-251: `isOSRCompilation` makes the OSR-context signal explicit.
    // Previously the self-recursion check below short-circuited silently
    // when currentCompilingFn was empty (i.e. in OSR), which meant every
    // method passed the self-recursion guard in OSR. The check is still a
    // no-op in OSR (there is no static caller name to compare against),
    // but the call site now communicates intent rather than relying on the
    // emptiness of an unrelated string.
    // MYT-173 follow-up: `perEntryDecisions` out-param. When non-null, each
    // slot [0..cache.entryCount) receives the per-entry eligibility result.
    // The site-level return is INLINE if at least one entry was inlineable
    // (the emitter routes each shape independently — inlineable entries
    // emit inline bodies, the rest route through the per-shape helper
    // branch in emitInlinedMethodCallPoly). Slots beyond entryCount are
    // left at the caller's default-initialised value.
    InlineDecision checkInlineEligibility(
        const vm::bytecode::BytecodeProgram& program,
        const vm::jit::ic::MethodInlineCache& cache,
        const std::string& currentCompilingFn,
        size_t currentInlineDepth,
        bool isOSRCompilation = false,
        std::array<InlineDecision, vm::jit::ic::IC_MAX_POLYMORPHIC_ENTRIES>*
            perEntryDecisions = nullptr);

    // MYT-210: plain-CALL / CALL_FAST inlining eligibility. Mirrors
    // checkInlineEligibility but the callee is statically known (no IC, no
    // receiver shape, no MONO/POLY split). Reuses the same per-callee gate
    // helpers — size, opcode scan, recursion, native, void-return.
    InlineDecision checkFunctionInlineEligibility(
        const vm::bytecode::BytecodeProgram& program,
        const vm::bytecode::BytecodeProgram::FunctionMetadata& callee,
        const std::string& currentCompilingFn,
        size_t currentInlineDepth);

    // MYT-210: human-readable name for telemetry / --jit-stats output. Mirrors
    // osrBailoutReasonName in shape. One-to-one with the InlineDecision enum;
    // returns "UNKNOWN" for out-of-range values (defensive).
    const char* inlineDecisionName(InlineDecision d);
}
