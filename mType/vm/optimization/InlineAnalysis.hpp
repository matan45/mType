#pragma once

#include "../bytecode/BytecodeProgram.hpp"

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
    // MYT-252: set to 0 (speculative inlining disabled entirely) as a
    // hard workaround. Two distinct bugs were on the path to a healthy
    // LIMIT >= 1:
    //
    //  (a) GET_FIELD on a primitive field in usesBoxedTypes mode wrote
    //      only to boxedBase, but downstream NOT / JUMP_IF_FALSE /
    //      JUMP_IF_TRUE / arith read raw primitives from stackBase —
    //      CALL_METHOD's slow path mirrors via emitReturnValueCopyBoxed
    //      (MYT-154); GET_FIELD had no equivalent. Fixed in
    //      emitGetFieldHelperInvoke + tryEmitInlinedFieldGet (this branch).
    //      Verified at LIMIT=2 with N=5 (single-iteration JIT scenarios).
    //
    //  (b) A separate multi-iteration hang remains on stream_pipeline_hot
    //      (N >= ~50) at LIMIT >= 1: count()'s second OSR'd invocation
    //      reads FilteringIterator's hasNextElement field as permanently
    //      true, looping forever. SET_FIELD on hasNextElement is observed
    //      to never reach jit_field_set_at or jit_set_field_ic — advance()
    //      is running entirely in the interpreter, but the JIT'd count()
    //      loop's view of hasNextElement isn't being invalidated. Likely
    //      class of bug: OSR re-entry / shape-cache invalidation across
    //      function-call boundaries (cousin of MYT-248/249/250 silent-OSR
    //      issues). Tracked as MYT-254. Disabling inlining (this constant
    //      = 0), nested inlining only (= 1), or OSR (MTYPE_DISABLE_OSR=1)
    //      all clear the hang.
    //
    // The "InlineDepthGuard" hypothesis from earlier comment iterations
    // was unrelated and stays reverted.
    //
    // Re-raising this constant (back to 1 for depth-1 wins, or 2 for the
    // original F-b nested-inline target) requires MYT-254 to be rooted
    // first — even depth-1 reproduces the multi-iter hang on this branch.
    constexpr size_t INLINE_DEPTH_LIMIT = 0;

    // MYT-251: `isOSRCompilation` makes the OSR-context signal explicit.
    // Previously the self-recursion check below short-circuited silently
    // when currentCompilingFn was empty (i.e. in OSR), which meant every
    // method passed the self-recursion guard in OSR. The check is still a
    // no-op in OSR (there is no static caller name to compare against),
    // but the call site now communicates intent rather than relying on the
    // emptiness of an unrelated string.
    InlineDecision checkInlineEligibility(
        const vm::bytecode::BytecodeProgram& program,
        const vm::jit::ic::MethodInlineCache& cache,
        const std::string& currentCompilingFn,
        size_t currentInlineDepth,
        bool isOSRCompilation = false);

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
