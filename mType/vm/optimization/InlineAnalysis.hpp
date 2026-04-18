#pragma once

#include <cstddef>
#include <string>

namespace vm::bytecode { class BytecodeProgram; }

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
        VALUE_OBJECT_RECEIVER,
        CALLEE_NATIVE,
        CALLEE_NOT_FOUND
    };

    // Size gate — callee bytecode instruction count. Chosen small so the
    // emitted code stays dense; F-a callees are tiny getters / arithmetic.
    constexpr size_t INLINE_SIZE_LIMIT = 16;

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
}
