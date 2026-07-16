#include "JitFrameAnalysis_Internal.hpp"

namespace vm::jit::analysis::detail
{
    bool TypedCfgAnalyzer::validateLinearEmitterModel(
        size_t rangeStart,
        size_t rangeEnd,
        const std::vector<PendingState>& cfgStates,
        AbstractState linearState,
        FrameAnalysisResult& result)
    {
        for (size_t ip = rangeStart; ip < rangeEnd; ++ip)
        {
            const PendingState& cfgState = cfgStates[ip - rangeStart];
            if (cfgState.initialized &&
                cfgState.state.stack.size() != linearState.stack.size())
            {
                setFailure(result,
                    FrameAnalysisStatus::EMITTER_STACK_MODEL_MISMATCH, ip);
                return false;
            }

            updateBasePeak(result, linearState.stack.size());
            if (!applyLinearEmitterInstruction(
                    program.getInstruction(ip), ip,
                    rangeStart, rangeEnd, linearState, result))
                return false;
            updateBasePeak(result, linearState.stack.size());
        }
        return true;
    }

    bool TypedCfgAnalyzer::applyLinearEmitterInstruction(
        const Instruction& instruction,
        size_t ip,
        size_t rangeStart,
        size_t rangeEnd,
        AbstractState& state,
        FrameAnalysisResult& result)
    {
        switch (instruction.opcode)
        {
            case OpCode::JUMP:
            case OpCode::JUMP_BACK:
                return validateLinearBranch(
                    instruction, ip, rangeStart, rangeEnd, result);
            case OpCode::JUMP_IF_FALSE:
            case OpCode::JUMP_IF_TRUE:
            case OpCode::JUMP_IF_FALSE_OR_POP:
            case OpCode::JUMP_IF_TRUE_OR_POP:
                if (!validateLinearBranch(
                        instruction, ip, rangeStart, rangeEnd, result))
                    return false;
                if (!popValues(state, 1))
                    return failEmitterModel(result, ip);
                return true;
            case OpCode::RETURN:
                return true;
            case OpCode::RETURN_VALUE:
            case OpCode::CREATE_PROMISE_RETURN_VALUE:
                if (!popValues(state, 1))
                    return failEmitterModel(result, ip);
                return true;
            default:
                break;
        }

        FrameAnalysisResult transferResult;
        if (!transferInstruction(instruction, state, ip, transferResult))
            return failEmitterModel(result, ip);
        return true;
    }

    bool TypedCfgAnalyzer::validateLinearBranch(
        const Instruction& instruction,
        size_t ip,
        size_t rangeStart,
        size_t rangeEnd,
        FrameAnalysisResult& result) const
    {
        if (!hasOperands(instruction, 1))
            return failEmitterModel(result, ip);
        const size_t target = static_cast<size_t>(
            instruction.inlineOperands[0]);
        if (target >= program.getInstructionCount())
            return failEmitterModel(result, ip);
        if (!allowExternalBranchTargets &&
            (target < rangeStart || target >= rangeEnd))
            return failEmitterModel(result, ip);
        return true;
    }

    bool TypedCfgAnalyzer::failEmitterModel(
        FrameAnalysisResult& result, size_t ip)
    {
        setFailure(result,
            FrameAnalysisStatus::EMITTER_STACK_MODEL_MISMATCH, ip);
        return false;
    }

    void TypedCfgAnalyzer::processInstruction(
        const Instruction& instruction,
        size_t ip,
        const AbstractState& input,
        size_t rangeStart,
        size_t rangeEnd,
        std::vector<PendingState>& states,
        std::deque<size_t>& worklist,
        FrameAnalysisResult& result)
    {
        if (processControlInstruction(
                instruction, ip, input, rangeStart, rangeEnd,
                states, worklist, result))
            return;
        if (!result.proven()) return;

        AbstractState output = input;
        if (!transferInstruction(instruction, output, ip, result)) return;
        updateBasePeak(result, output.stack.size());
        if (ip + 1 < rangeEnd)
            enqueue(states, worklist, rangeStart, rangeEnd,
                    ip + 1, output, result);
    }

    bool TypedCfgAnalyzer::processControlInstruction(
        const Instruction& instruction,
        size_t ip,
        const AbstractState& input,
        size_t rangeStart,
        size_t rangeEnd,
        std::vector<PendingState>& states,
        std::deque<size_t>& worklist,
        FrameAnalysisResult& result)
    {
        switch (instruction.opcode)
        {
            case OpCode::JUMP:
            case OpCode::JUMP_BACK:
                return processUnconditionalBranch(
                    instruction, input, rangeStart, rangeEnd,
                    states, worklist, result, ip);
            case OpCode::JUMP_IF_FALSE:
            case OpCode::JUMP_IF_TRUE:
                return processPoppingBranch(
                    instruction, input, rangeStart, rangeEnd,
                    states, worklist, result, ip);
            case OpCode::JUMP_IF_FALSE_OR_POP:
            case OpCode::JUMP_IF_TRUE_OR_POP:
                return processShortCircuitBranch(
                    instruction, input, rangeStart, rangeEnd,
                    states, worklist, result, ip);
            case OpCode::RETURN:
                return true;
            case OpCode::RETURN_VALUE:
            case OpCode::CREATE_PROMISE_RETURN_VALUE:
                if (!requireTop(input))
                    setFailure(result,
                        FrameAnalysisStatus::STACK_UNDERFLOW, ip);
                return true;
            default:
                return false;
        }
    }

    bool TypedCfgAnalyzer::processUnconditionalBranch(
        const Instruction& instruction,
        const AbstractState& input,
        size_t rangeStart,
        size_t rangeEnd,
        std::vector<PendingState>& states,
        std::deque<size_t>& worklist,
        FrameAnalysisResult& result,
        size_t ip)
    {
        if (!hasOperands(instruction, 1))
        {
            setFailure(result, FrameAnalysisStatus::INVALID_OPERANDS, ip);
            return true;
        }
        enqueue(states, worklist, rangeStart, rangeEnd,
                static_cast<size_t>(instruction.inlineOperands[0]),
                input, result);
        return true;
    }

    bool TypedCfgAnalyzer::processPoppingBranch(
        const Instruction& instruction,
        const AbstractState& input,
        size_t rangeStart,
        size_t rangeEnd,
        std::vector<PendingState>& states,
        std::deque<size_t>& worklist,
        FrameAnalysisResult& result,
        size_t ip)
    {
        if (!hasOperands(instruction, 1))
        {
            setFailure(result, FrameAnalysisStatus::INVALID_OPERANDS, ip);
            return true;
        }
        AbstractState output = input;
        if (!popValues(output, 1))
        {
            setFailure(result, FrameAnalysisStatus::STACK_UNDERFLOW, ip);
            return true;
        }
        enqueue(states, worklist, rangeStart, rangeEnd,
                static_cast<size_t>(instruction.inlineOperands[0]),
                output, result);
        if (result.proven() && ip + 1 < rangeEnd)
            enqueue(states, worklist, rangeStart, rangeEnd,
                    ip + 1, output, result);
        return true;
    }

    bool TypedCfgAnalyzer::processShortCircuitBranch(
        const Instruction& instruction,
        const AbstractState& input,
        size_t rangeStart,
        size_t rangeEnd,
        std::vector<PendingState>& states,
        std::deque<size_t>& worklist,
        FrameAnalysisResult& result,
        size_t ip)
    {
        if (!hasOperands(instruction, 1))
        {
            setFailure(result, FrameAnalysisStatus::INVALID_OPERANDS, ip);
            return true;
        }
        if (!requireTop(input))
        {
            setFailure(result, FrameAnalysisStatus::STACK_UNDERFLOW, ip);
            return true;
        }

        enqueue(states, worklist, rangeStart, rangeEnd,
                static_cast<size_t>(instruction.inlineOperands[0]),
                input, result);
        AbstractState fallthrough = input;
        popValues(fallthrough, 1);
        if (result.proven() && ip + 1 < rangeEnd)
            enqueue(states, worklist, rangeStart, rangeEnd,
                    ip + 1, fallthrough, result);
        return true;
    }
}
