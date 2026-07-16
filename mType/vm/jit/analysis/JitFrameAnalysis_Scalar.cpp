#include "JitFrameAnalysis_Internal.hpp"

#include <utility>

namespace vm::jit::analysis::detail
{
    bool TypedCfgAnalyzer::transferInstruction(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (transferStackAndArithmetic(instruction, state, ip, result))
            return result.proven();
        if (!result.proven()) return false;
        if (transferLocalsAndCalls(instruction, state, ip, result))
            return result.proven();
        if (!result.proven()) return false;
        if (transferObjectsAndArrays(instruction, state, ip, result))
            return result.proven();
        if (!result.proven()) return false;
        if (transferPrimitiveInvoke(instruction, state, ip, result))
            return result.proven();

        setFailure(result,
            FrameAnalysisStatus::UNSUPPORTED_STACK_EFFECT, ip);
        return false;
    }

    bool TypedCfgAnalyzer::transferStackAndArithmetic(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        switch (instruction.opcode)
        {
            case OpCode::LINE: case OpCode::SOURCE_FILE:
            case OpCode::NOP: case OpCode::LOOP_START:
            case OpCode::LOOP_END: case OpCode::PROFILE_ENTER:
            case OpCode::PROFILE_EXIT: case OpCode::HALT:
                return true;
            case OpCode::PUSH_INT:
                return pushWithOperands(instruction, state,
                    AbstractSlotType::INT, ip, result);
            case OpCode::PUSH_FLOAT:
                return pushWithOperands(instruction, state,
                    AbstractSlotType::FLOAT, ip, result);
            case OpCode::PUSH_BOOL:
                return pushWithOperands(instruction, state,
                    AbstractSlotType::BOOL, ip, result);
            case OpCode::PUSH_STRING:
                return pushWithOperands(instruction, state,
                    AbstractSlotType::BOXED, ip, result);
            case OpCode::PUSH_NULL:
                state.stack.push_back(AbstractSlotType::BOXED);
                return true;
            case OpCode::POP:
                return popOrFail(state, 1, ip, result);
            case OpCode::DUP:
                if (!requireTop(state)) return failUnderflow(result, ip);
                state.stack.push_back(state.stack.back());
                return true;
            case OpCode::SWAP:
                if (state.stack.size() < 2)
                    return failUnderflow(result, ip);
                std::swap(state.stack[state.stack.size() - 1],
                          state.stack[state.stack.size() - 2]);
                return true;
            default:
                break;
        }
        return transferArithmeticOperation(
            instruction.opcode, state, ip, result);
    }

    bool TypedCfgAnalyzer::transferArithmeticOperation(
        OpCode opcode,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (isIntBinary(opcode))
            return replaceOrFail(
                state, 2, AbstractSlotType::INT, ip, result);
        if (isFloatBinary(opcode))
            return replaceOrFail(
                state, 2, AbstractSlotType::FLOAT, ip, result);
        if (isComparison(opcode) || isLogicalBinary(opcode))
            return replaceOrFail(
                state, 2, AbstractSlotType::BOOL, ip, result);
        if (isGenericBinary(opcode))
            return replaceOrFail(
                state, 2, AbstractSlotType::UNKNOWN, ip, result);
        if (isIntUnary(opcode))
            return replaceOrFail(
                state, 1,
                opcode == OpCode::NOT ? AbstractSlotType::BOOL
                                      : AbstractSlotType::INT,
                ip, result);
        if (opcode == OpCode::ADD_INT_CONST)
            return replaceOrFail(
                state, 1, AbstractSlotType::INT, ip, result);
        if (opcode == OpCode::LOAD_LOAD_ADD_INT ||
            opcode == OpCode::LOAD_LOAD_SUB_INT ||
            opcode == OpCode::LOAD_LOAD_MUL_INT)
        {
            state.stack.push_back(AbstractSlotType::INT);
            return true;
        }
        if (opcode == OpCode::ADD_INT_STORE_LOCAL)
            return transferAddStoreLocal(state, ip, result);
        return false;
    }

    bool TypedCfgAnalyzer::isIntBinary(OpCode opcode) noexcept
    {
        switch (opcode)
        {
            case OpCode::ADD_INT: case OpCode::SUB_INT:
            case OpCode::MUL_INT: case OpCode::DIV_INT:
            case OpCode::BITWISE_AND_OP: case OpCode::BITWISE_OR_OP:
            case OpCode::BITWISE_XOR_OP: case OpCode::LEFT_SHIFT_OP:
            case OpCode::RIGHT_SHIFT_OP: case OpCode::BITWISE_AND_INT:
            case OpCode::BITWISE_OR_INT: case OpCode::BITWISE_XOR_INT:
            case OpCode::LEFT_SHIFT_INT: case OpCode::RIGHT_SHIFT_INT:
                return true;
            default: return false;
        }
    }

    bool TypedCfgAnalyzer::isFloatBinary(OpCode opcode) noexcept
    {
        return opcode == OpCode::ADD_FLOAT || opcode == OpCode::SUB_FLOAT ||
               opcode == OpCode::MUL_FLOAT || opcode == OpCode::DIV_FLOAT;
    }

    bool TypedCfgAnalyzer::isGenericBinary(OpCode opcode) noexcept
    {
        return opcode == OpCode::ADD || opcode == OpCode::SUB ||
               opcode == OpCode::MUL || opcode == OpCode::DIV ||
               opcode == OpCode::MOD;
    }

    bool TypedCfgAnalyzer::isComparison(OpCode opcode) noexcept
    {
        switch (opcode)
        {
            case OpCode::EQ: case OpCode::NE: case OpCode::LT:
            case OpCode::GT: case OpCode::LE: case OpCode::GE:
            case OpCode::EQ_INT: case OpCode::NE_INT:
            case OpCode::LT_INT: case OpCode::GT_INT:
                return true;
            default: return false;
        }
    }

    bool TypedCfgAnalyzer::isLogicalBinary(OpCode opcode) noexcept
    {
        return opcode == OpCode::AND || opcode == OpCode::OR;
    }

    bool TypedCfgAnalyzer::isIntUnary(OpCode opcode) noexcept
    {
        return opcode == OpCode::NEG || opcode == OpCode::INC ||
               opcode == OpCode::DEC || opcode == OpCode::NOT ||
               opcode == OpCode::BITWISE_NOT_OP ||
               opcode == OpCode::BITWISE_NOT_INT;
    }

    bool TypedCfgAnalyzer::transferAddStoreLocal(
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        const auto& instruction = program.getInstruction(ip);
        if (!hasOperands(instruction, 1)) return failOperands(result, ip);
        if (!replaceOrFail(state, 2, AbstractSlotType::INT, ip, result))
            return false;
        return setLocal(
            state, instruction.inlineOperands[0],
            AbstractSlotType::INT, ip, result);
    }

    bool TypedCfgAnalyzer::transferLocalsAndCalls(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        switch (instruction.opcode)
        {
            case OpCode::LOAD_LOCAL: case OpCode::LOAD_LOCAL_INT:
            case OpCode::LOAD_LOCAL_FLOAT: case OpCode::LOAD_LOCAL_BOOL:
            case OpCode::LOAD_LOCAL_BOXED_INST:
                return transferLoadLocal(instruction, state, ip, result);
            case OpCode::STORE_LOCAL: case OpCode::STORE_LOCAL_INT:
            case OpCode::STORE_LOCAL_FLOAT: case OpCode::STORE_LOCAL_BOOL:
            case OpCode::STORE_LOCAL_BOXED_INST:
                return transferStoreLocal(instruction, state, ip, result);
            case OpCode::LOAD_STORE_LOCAL:
                return transferLoadStoreLocal(instruction, state, ip, result);
            case OpCode::LOAD_VAR: case OpCode::LOAD_VAR_CACHED:
                state.stack.push_back(AbstractSlotType::BOXED);
                return true;
            case OpCode::STORE_VAR: case OpCode::STORE_VAR_CACHED:
                return requireOrFail(state, ip, result);
            case OpCode::DECLARE_VAR:
                return popOrFail(state, 1, ip, result);
            case OpCode::CALL:
                return transferNamedCall(instruction, state, ip, result);
            case OpCode::CALL_FAST:
                return transferFastCall(instruction, state, ip, result);
            case OpCode::CALL_STATIC:
                return transferStaticCall(instruction, state, ip, result);
            case OpCode::CALL_METHOD:
            case OpCode::CALL_METHOD_CACHED:
            case OpCode::CALL_METHOD_POLY_CACHED:
                return transferMethodCall(
                    instruction, state, ip, result, false);
            case OpCode::LOAD_LOCAL_CALL_CACHED:
            case OpCode::LOAD_LOCAL_CALL_POLY_CACHED:
                return transferMethodCall(
                    instruction, state, ip, result, true);
            default:
                return false;
        }
    }

    bool TypedCfgAnalyzer::transferLoadLocal(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (!hasOperands(instruction, 1)) return failOperands(result, ip);
        const size_t slot = static_cast<size_t>(instruction.inlineOperands[0]);
        if (slot >= state.locals.size()) return failLocal(result, ip);
        AbstractSlotType type = forcedLocalType(instruction.opcode);
        if (type == AbstractSlotType::UNKNOWN) type = state.locals[slot];
        state.stack.push_back(type);
        return true;
    }

    bool TypedCfgAnalyzer::transferStoreLocal(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (!hasOperands(instruction, 1)) return failOperands(result, ip);
        if (!requireTop(state)) return failUnderflow(result, ip);
        AbstractSlotType type = forcedLocalType(instruction.opcode);
        if (type == AbstractSlotType::UNKNOWN) type = state.stack.back();
        return setLocal(
            state, instruction.inlineOperands[0], type, ip, result);
    }

    bool TypedCfgAnalyzer::transferLoadStoreLocal(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (!hasOperands(instruction, 2)) return failOperands(result, ip);
        const size_t source = static_cast<size_t>(instruction.inlineOperands[0]);
        const size_t destination = static_cast<size_t>(instruction.inlineOperands[1]);
        if (source >= state.locals.size() || destination >= state.locals.size())
            return failLocal(result, ip);
        const AbstractSlotType type = state.locals[source];
        state.stack.push_back(type);
        state.locals[destination] = type;
        return true;
    }

    bool TypedCfgAnalyzer::transferNamedCall(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (!hasOperands(instruction, 2)) return failOperands(result, ip);
        const auto* callee = namedCallee(instruction);
        if (!callee)
        {
            const uint32_t nameIndex = static_cast<uint32_t>(
                instruction.inlineOperands[0]);
            if (nameIndex < program.getConstantPool().strings.size())
            {
                const std::string& name =
                    program.getConstantPool().getString(nameIndex);
                if (name == "print" || name == "println")
                    return popOrFail(
                        state,
                        static_cast<size_t>(instruction.inlineOperands[1]),
                        ip, result);
            }
            return failCallee(result, ip);
        }
        return transferResolvedCall(
            state, static_cast<size_t>(instruction.inlineOperands[1]),
            callee->returnType, ip, result);
    }

    bool TypedCfgAnalyzer::transferFastCall(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (!hasOperands(instruction, 2)) return failOperands(result, ip);
        const auto* callee = program.getFunctionByIndex(
            static_cast<size_t>(instruction.inlineOperands[0]));
        if (!callee) return failCallee(result, ip);
        return transferResolvedCall(
            state, static_cast<size_t>(instruction.inlineOperands[1]),
            callee->returnType, ip, result);
    }

    bool TypedCfgAnalyzer::transferStaticCall(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (!hasOperands(instruction, 2)) return failOperands(result, ip);
        return replaceOrFail(
            state, static_cast<size_t>(instruction.inlineOperands[1]),
            AbstractSlotType::BOXED, ip, result);
    }

    bool TypedCfgAnalyzer::transferMethodCall(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result,
        bool fusedLocalLoad)
    {
        if (!hasOperands(instruction, 2)) return failOperands(result, ip);
        if (fusedLocalLoad)
            state.stack.push_back(AbstractSlotType::BOXED);
        const size_t args = static_cast<size_t>(instruction.inlineOperands[1]);
        if (args >= state.stack.size()) return failUnderflow(result, ip);
        return replaceOrFail(
            state, args + 1, AbstractSlotType::BOXED, ip, result);
    }

    bool TypedCfgAnalyzer::transferResolvedCall(
        AbstractState& state,
        size_t args,
        const std::string& returnType,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (!popValues(state, args)) return failUnderflow(result, ip);
        if (returnType != "void")
            state.stack.push_back(typeFromName(returnType));
        return true;
    }

    bool TypedCfgAnalyzer::pushWithOperands(
        const Instruction& instruction,
        AbstractState& state,
        AbstractSlotType type,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (!hasOperands(instruction, 1)) return failOperands(result, ip);
        state.stack.push_back(type);
        return true;
    }

    bool TypedCfgAnalyzer::requireOrFail(
        const AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (requireTop(state)) return true;
        return failUnderflow(result, ip);
    }

    bool TypedCfgAnalyzer::popOrFail(
        AbstractState& state,
        size_t count,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (popValues(state, count)) return true;
        return failUnderflow(result, ip);
    }

    bool TypedCfgAnalyzer::replaceOrFail(
        AbstractState& state,
        size_t consumed,
        AbstractSlotType produced,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (replaceValues(state, consumed, produced)) return true;
        return failUnderflow(result, ip);
    }

    bool TypedCfgAnalyzer::setLocal(
        AbstractState& state,
        uint64_t slotValue,
        AbstractSlotType type,
        size_t ip,
        FrameAnalysisResult& result)
    {
        const size_t slot = static_cast<size_t>(slotValue);
        if (slot >= state.locals.size()) return failLocal(result, ip);
        state.locals[slot] = type;
        return true;
    }

    bool TypedCfgAnalyzer::failUnderflow(
        FrameAnalysisResult& result, size_t ip)
    {
        setFailure(result, FrameAnalysisStatus::STACK_UNDERFLOW, ip);
        return false;
    }

    bool TypedCfgAnalyzer::failOperands(
        FrameAnalysisResult& result, size_t ip)
    {
        setFailure(result, FrameAnalysisStatus::INVALID_OPERANDS, ip);
        return false;
    }

    bool TypedCfgAnalyzer::failLocal(
        FrameAnalysisResult& result, size_t ip)
    {
        setFailure(result, FrameAnalysisStatus::INVALID_LOCAL_SLOT, ip);
        return false;
    }

    bool TypedCfgAnalyzer::failCallee(
        FrameAnalysisResult& result, size_t ip)
    {
        setFailure(result, FrameAnalysisStatus::UNRESOLVED_CALLEE, ip);
        return false;
    }
}
