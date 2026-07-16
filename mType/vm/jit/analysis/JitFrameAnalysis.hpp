#pragma once

#include "../../bytecode/BytecodeProgram.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace vm::jit::ic
{
    class TypeFeedbackCollector;
}

namespace vm::jit::analysis
{
    // The frame planner fails closed. A proven result can be tightly
    // allocated. An unmodelled but otherwise emittable effect selects the
    // historical hard-cap layout; malformed bytecode or an emitter-incompatible
    // CFG rejects native compilation.
    enum class FrameAnalysisStatus : uint8_t
    {
        PROVEN,
        INVALID_RANGE,
        INVALID_OPERANDS,
        UNSUPPORTED_STACK_EFFECT,
        STACK_UNDERFLOW,
        STACK_DEPTH_MERGE_MISMATCH,
        INVALID_BRANCH_TARGET,
        INVALID_LOCAL_SLOT,
        UNRESOLVED_CALLEE,
        CROSS_PROGRAM_INLINE,
        EMITTER_STACK_MODEL_MISMATCH,
        INLINE_FRAME_LIMIT_EXCEEDED,
    };

    struct FrameAnalysisResult
    {
        FrameAnalysisStatus status = FrameAnalysisStatus::PROVEN;
        // Peak of the bytecode range itself, excluding pasted inline bodies.
        size_t baseOperandStackPeak = 0;
        // Peak including inline bodies allowed by the current IC snapshot.
        size_t operandStackPeak = 0;
        // Extra local slots beyond the owning frame's localCount.
        size_t inlineLocalSlots = 0;
        size_t offendingOffset = std::numeric_limits<size_t>::max();
        size_t typeWideningCount = 0;

        bool proven() const noexcept
        {
            return status == FrameAnalysisStatus::PROVEN;
        }
    };

    struct FrameLayoutPlan
    {
        bool valid = true;
        bool usedConservativeFallback = false;
        size_t operandStackSlots = 0;
        size_t inlineLocalSlots = 0;
        size_t analyzedOperandPeak = 0;
        FrameAnalysisStatus analysisStatus = FrameAnalysisStatus::PROVEN;
    };

    // Layout guard bands for emitter-only scratch traffic.
    inline constexpr size_t OPERAND_STACK_SAFETY_MARGIN = 4;
    inline constexpr size_t MIN_OPERAND_STACK_SLOTS = 8;
    inline constexpr size_t INLINE_LOCAL_SAFETY_MARGIN = 2;
    inline constexpr size_t DEFAULT_INLINE_LOCAL_HARD_LIMIT = 96;

    FrameLayoutPlan makeFrameLayoutPlan(
        const FrameAnalysisResult& result,
        size_t hardOperandLimit,
        size_t hardInlineLocalLimit);

    FrameAnalysisResult analyzeFunctionFrame(
        const bytecode::BytecodeProgram& program,
        const bytecode::BytecodeProgram::FunctionMetadata& function,
        const ic::TypeFeedbackCollector* typeFeedback,
        bool usesBoxedTypes,
        size_t hardInlineLocalLimit = DEFAULT_INLINE_LOCAL_HARD_LIMIT);

    FrameAnalysisResult analyzeOSRFrame(
        const bytecode::BytecodeProgram& program,
        size_t loopStartOffset,
        size_t loopEndOffsetInclusive,
        size_t localCount,
        const ic::TypeFeedbackCollector* typeFeedback,
        bool usesBoxedTypes,
        size_t hardInlineLocalLimit = DEFAULT_INLINE_LOCAL_HARD_LIMIT);

    FrameAnalysisResult analyzeInlinedCalleeFrame(
        const bytecode::BytecodeProgram& program,
        const bytecode::BytecodeProgram::FunctionMetadata& callee,
        const ic::TypeFeedbackCollector* typeFeedback,
        bool usesBoxedTypes,
        const std::string& currentCompilingFunction,
        bool isOSRCompilation,
        size_t inlineDepth,
        size_t hardInlineLocalLimit = DEFAULT_INLINE_LOCAL_HARD_LIMIT);
}
