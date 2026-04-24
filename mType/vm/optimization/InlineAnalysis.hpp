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
    // (inlineStack.size()). F-a ships with depth 1 only (restriction forbids
    // nested CALL); kept at 2 so F-b can unlock nested inlining without
    // re-checking the constant.
    constexpr size_t INLINE_DEPTH_LIMIT = 2;

    InlineDecision checkInlineEligibility(
        const vm::bytecode::BytecodeProgram& program,
        const vm::jit::ic::MethodInlineCache& cache,
        const std::string& currentCompilingFn,
        size_t currentInlineDepth);

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
