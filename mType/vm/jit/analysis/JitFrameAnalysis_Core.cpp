#include "JitFrameAnalysis_Internal.hpp"
#include "../../../constants/SecurityConstants.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace vm::jit::analysis
{
    FrameLayoutPlan makeFrameLayoutPlan(
        const FrameAnalysisResult& result,
        size_t hardOperandLimit,
        size_t hardInlineLocalLimit)
    {
        FrameLayoutPlan plan;
        plan.analysisStatus = result.status;
        plan.analyzedOperandPeak = result.operandStackPeak;

        if (!result.proven())
        {
            if (result.status !=
                    FrameAnalysisStatus::UNSUPPORTED_STACK_EFFECT &&
                result.status != FrameAnalysisStatus::CROSS_PROGRAM_INLINE)
            {
                plan.valid = false;
                return plan;
            }
            plan.usedConservativeFallback = true;
            plan.operandStackSlots = hardOperandLimit;
            plan.inlineLocalSlots = hardInlineLocalLimit;
            return plan;
        }

        if (result.baseOperandStackPeak > hardOperandLimit)
        {
            plan.valid = false;
            return plan;
        }

        const size_t peakWithMargin = result.operandStackPeak >
            std::numeric_limits<size_t>::max() - OPERAND_STACK_SAFETY_MARGIN
            ? std::numeric_limits<size_t>::max()
            : result.operandStackPeak + OPERAND_STACK_SAFETY_MARGIN;
        plan.operandStackSlots = std::min(
            hardOperandLimit,
            std::max(MIN_OPERAND_STACK_SLOTS, peakWithMargin));

        if (result.inlineLocalSlots != 0)
        {
            const size_t localsWithMargin = result.inlineLocalSlots >
                std::numeric_limits<size_t>::max() - INLINE_LOCAL_SAFETY_MARGIN
                ? std::numeric_limits<size_t>::max()
                : result.inlineLocalSlots + INLINE_LOCAL_SAFETY_MARGIN;
            plan.inlineLocalSlots = std::min(
                hardInlineLocalLimit, localsWithMargin);
        }
        return plan;
    }

    FrameAnalysisResult analyzeFunctionFrame(
        const bytecode::BytecodeProgram& program,
        const bytecode::BytecodeProgram::FunctionMetadata& function,
        const ic::TypeFeedbackCollector* typeFeedback,
        bool usesBoxedTypes,
        size_t hardInlineLocalLimit)
    {
        const std::string& identity = function.mangledName.empty()
            ? function.name : function.mangledName;
        detail::TypedCfgAnalyzer analyzer(
            program, typeFeedback, usesBoxedTypes, identity,
            /*isOSRCompilation=*/false, hardInlineLocalLimit);
        return analyzer.analyzeFunction(function);
    }

    FrameAnalysisResult analyzeOSRFrame(
        const bytecode::BytecodeProgram& program,
        size_t loopStartOffset,
        size_t loopEndOffsetInclusive,
        size_t localCount,
        const ic::TypeFeedbackCollector* typeFeedback,
        bool usesBoxedTypes,
        size_t hardInlineLocalLimit)
    {
        if (loopEndOffsetInclusive == std::numeric_limits<size_t>::max())
        {
            FrameAnalysisResult result;
            result.status = FrameAnalysisStatus::INVALID_RANGE;
            result.offendingOffset = loopStartOffset;
            return result;
        }
        detail::TypedCfgAnalyzer analyzer(
            program, typeFeedback, usesBoxedTypes, std::string(),
            /*isOSRCompilation=*/true, hardInlineLocalLimit);
        return analyzer.analyzeOSRRange(
            loopStartOffset, loopEndOffsetInclusive + 1, localCount);
    }

    FrameAnalysisResult analyzeInlinedCalleeFrame(
        const bytecode::BytecodeProgram& program,
        const bytecode::BytecodeProgram::FunctionMetadata& callee,
        const ic::TypeFeedbackCollector* typeFeedback,
        bool usesBoxedTypes,
        const std::string& currentCompilingFunction,
        bool isOSRCompilation,
        size_t inlineDepth,
        size_t hardInlineLocalLimit)
    {
        detail::TypedCfgAnalyzer analyzer(
            program, typeFeedback, usesBoxedTypes,
            currentCompilingFunction, isOSRCompilation,
            hardInlineLocalLimit);
        return analyzer.analyzeFunction(
            callee, inlineDepth, /*owningFrameMinimumLocal=*/false);
    }
}

namespace vm::jit::analysis::detail
{
    AbstractSlotType typeFromName(const std::string& type)
    {
        if (type == "int") return AbstractSlotType::INT;
        if (type == "float") return AbstractSlotType::FLOAT;
        if (type == "bool") return AbstractSlotType::BOOL;
        if (type.empty() || type == "void") return AbstractSlotType::UNKNOWN;
        return AbstractSlotType::BOXED;
    }

    AbstractSlotType forcedLocalType(bytecode::OpCode opcode)
    {
        using bytecode::OpCode;
        switch (opcode)
        {
            case OpCode::LOAD_LOCAL_INT:
            case OpCode::STORE_LOCAL_INT: return AbstractSlotType::INT;
            case OpCode::LOAD_LOCAL_FLOAT:
            case OpCode::STORE_LOCAL_FLOAT: return AbstractSlotType::FLOAT;
            case OpCode::LOAD_LOCAL_BOOL:
            case OpCode::STORE_LOCAL_BOOL: return AbstractSlotType::BOOL;
            case OpCode::LOAD_LOCAL_BOXED_INST:
            case OpCode::STORE_LOCAL_BOXED_INST: return AbstractSlotType::BOXED;
            default: return AbstractSlotType::UNKNOWN;
        }
    }

    bool hasOperands(
        const bytecode::BytecodeProgram::Instruction& instruction,
        size_t required) noexcept
    {
        return instruction.numOperands() >= required;
    }

    bool popValues(AbstractState& state, size_t count)
    {
        if (state.stack.size() < count) return false;
        state.stack.resize(state.stack.size() - count);
        return true;
    }

    bool replaceValues(
        AbstractState& state, size_t consumed, AbstractSlotType produced)
    {
        if (!popValues(state, consumed)) return false;
        state.stack.push_back(produced);
        return true;
    }

    bool requireTop(const AbstractState& state) noexcept
    {
        return !state.stack.empty();
    }

    TypedCfgAnalyzer::TypedCfgAnalyzer(
        const bytecode::BytecodeProgram& program,
        const ic::TypeFeedbackCollector* typeFeedback,
        bool usesBoxedTypes,
        std::string currentCompilingFunction,
        bool isOSRCompilation,
        size_t hardInlineLocalLimit)
        : program(program),
          typeFeedback(typeFeedback),
          usesBoxedTypes(usesBoxedTypes),
          currentCompilingFunction(std::move(currentCompilingFunction)),
          isOSRCompilation(isOSRCompilation),
          hardInlineLocalLimit(hardInlineLocalLimit)
    {
    }

    FrameAnalysisResult TypedCfgAnalyzer::analyzeFunction(
        const bytecode::BytecodeProgram::FunctionMetadata& function,
        size_t inlineDepth,
        bool owningFrameMinimumLocal)
    {
        if (function.localCount >
                constants::security::MAX_LOCAL_STACK_PER_FRAME ||
            function.parameterCount > function.localCount)
        {
            FrameAnalysisResult result;
            result.status = FrameAnalysisStatus::INVALID_LOCAL_SLOT;
            result.offendingOffset = function.startOffset;
            return result;
        }
        if (!owningFrameMinimumLocal &&
            function.localCount > hardInlineLocalLimit)
        {
            FrameAnalysisResult result;
            result.status = FrameAnalysisStatus::INLINE_FRAME_LIMIT_EXCEEDED;
            result.offendingOffset = function.startOffset;
            return result;
        }
        const size_t localCount = owningFrameMinimumLocal
            ? std::max<size_t>(1, function.localCount)
            : function.localCount;
        AbstractState entry;
        entry.locals.assign(localCount, AbstractSlotType::UNKNOWN);
        const size_t parameterCount = std::min(
            {function.parameterCount, function.parameterTypes.size(), localCount});
        for (size_t i = 0; i < parameterCount; ++i)
            entry.locals[i] = typeFromName(function.parameterTypes[i]);
        return analyzeRange(
            function.startOffset,
            function.startOffset + function.instructionCount,
            std::move(entry), inlineDepth,
            /*externalBranchTargetsAllowed=*/false);
    }

    FrameAnalysisResult TypedCfgAnalyzer::analyzeOSRRange(
        size_t startOffset,
        size_t endOffsetExclusive,
        size_t localCount)
    {
        if (localCount > constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            FrameAnalysisResult result;
            result.status = FrameAnalysisStatus::INVALID_LOCAL_SLOT;
            result.offendingOffset = startOffset;
            return result;
        }
        AbstractState entry;
        entry.locals.assign(
            std::max<size_t>(1, localCount), AbstractSlotType::UNKNOWN);
        return analyzeRange(
            startOffset, endOffsetExclusive, std::move(entry), 0,
            /*externalBranchTargetsAllowed=*/true);
    }

    bool TypedCfgAnalyzer::isInlineableDecision(
        optimization::InlineDecision decision) noexcept
    {
        return decision == optimization::InlineDecision::INLINE ||
               decision == optimization::InlineDecision::
                   INLINE_VALUE_REQUIRES_MATERIALISATION;
    }

    void TypedCfgAnalyzer::setFailure(
        FrameAnalysisResult& result,
        FrameAnalysisStatus status,
        size_t offset)
    {
        if (!result.proven()) return;
        result.status = status;
        result.offendingOffset = offset;
    }

    bool TypedCfgAnalyzer::mergeState(
        PendingState& destination,
        const AbstractState& source,
        FrameAnalysisResult& result,
        size_t targetOffset)
    {
        if (!destination.initialized)
        {
            destination.initialized = true;
            destination.state = source;
            return true;
        }
        if (destination.state.stack.size() != source.stack.size())
        {
            setFailure(result,
                FrameAnalysisStatus::STACK_DEPTH_MERGE_MISMATCH,
                targetOffset);
            return false;
        }
        if (destination.state.locals.size() != source.locals.size())
        {
            setFailure(result,
                FrameAnalysisStatus::INVALID_LOCAL_SLOT, targetOffset);
            return false;
        }

        bool changed = false;
        changed |= widenTypes(destination.state.stack, source.stack, result);
        changed |= widenTypes(destination.state.locals, source.locals, result);
        return changed;
    }

    bool TypedCfgAnalyzer::widenTypes(
        std::vector<AbstractSlotType>& destination,
        const std::vector<AbstractSlotType>& source,
        FrameAnalysisResult& result)
    {
        bool changed = false;
        for (size_t i = 0; i < destination.size(); ++i)
        {
            if (destination[i] == source[i] ||
                destination[i] == AbstractSlotType::UNKNOWN)
                continue;
            destination[i] = AbstractSlotType::UNKNOWN;
            ++result.typeWideningCount;
            changed = true;
        }
        return changed;
    }

    bool TypedCfgAnalyzer::enqueue(
        std::vector<PendingState>& states,
        std::deque<size_t>& worklist,
        size_t rangeStart,
        size_t rangeEnd,
        size_t target,
        const AbstractState& state,
        FrameAnalysisResult& result)
    {
        if (target >= program.getInstructionCount())
        {
            setFailure(result,
                FrameAnalysisStatus::INVALID_BRANCH_TARGET, target);
            return false;
        }
        if (target < rangeStart || target >= rangeEnd)
        {
            if (!allowExternalBranchTargets)
            {
                setFailure(result,
                    FrameAnalysisStatus::INVALID_BRANCH_TARGET, target);
                return false;
            }
            return true;
        }

        PendingState& pending = states[target - rangeStart];
        const bool changed = mergeState(pending, state, result, target);
        if (!result.proven()) return false;
        if (changed && !pending.queued)
        {
            pending.queued = true;
            worklist.push_back(target);
        }
        return true;
    }

    FrameAnalysisResult TypedCfgAnalyzer::analyzeRange(
        size_t rangeStart,
        size_t rangeEnd,
        AbstractState entry,
        size_t inlineDepth,
        bool externalBranchTargetsAllowed)
    {
        FrameAnalysisResult result;
        const size_t instructionCount = program.getInstructionCount();
        if (rangeStart > rangeEnd || rangeEnd > instructionCount ||
            rangeStart == rangeEnd)
        {
            setFailure(result, FrameAnalysisStatus::INVALID_RANGE, rangeStart);
            return result;
        }

        const bool previousExternalBranchPolicy = allowExternalBranchTargets;
        allowExternalBranchTargets = externalBranchTargetsAllowed;
        std::vector<PendingState> states(rangeEnd - rangeStart);
        std::deque<size_t> worklist;
        AbstractState linearEntry = entry;
        states[0].initialized = true;
        states[0].queued = true;
        states[0].state = std::move(entry);
        worklist.push_back(rangeStart);

        while (!worklist.empty() && result.proven())
        {
            const size_t ip = worklist.front();
            worklist.pop_front();
            PendingState& pending = states[ip - rangeStart];
            pending.queued = false;
            const AbstractState input = pending.state;

            updateBasePeak(result, input.stack.size());
            if (!collectInlineRequirements(
                    program.getInstruction(ip), ip, input.stack.size(),
                    inlineDepth, result))
                break;
            processInstruction(
                program.getInstruction(ip), ip, input,
                rangeStart, rangeEnd, states, worklist, result);
        }
        if (result.proven())
        {
            validateLinearEmitterModel(
                rangeStart, rangeEnd, states,
                std::move(linearEntry), result);
        }
        allowExternalBranchTargets = previousExternalBranchPolicy;
        return result;
    }

    void TypedCfgAnalyzer::updateBasePeak(
        FrameAnalysisResult& result, size_t depth)
    {
        result.baseOperandStackPeak = std::max(
            result.baseOperandStackPeak, depth);
        result.operandStackPeak = std::max(result.operandStackPeak, depth);
    }
}
