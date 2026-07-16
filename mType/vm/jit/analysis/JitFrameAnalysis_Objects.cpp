#include "JitFrameAnalysis_Internal.hpp"

namespace vm::jit::analysis::detail
{
    bool TypedCfgAnalyzer::transferObjectsAndArrays(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (transferFieldOperation(instruction, state, ip, result))
            return result.proven();
        if (!result.proven()) return false;
        if (transferObjectOperation(instruction, state, ip, result))
            return result.proven();
        if (!result.proven()) return false;
        if (transferArrayOperation(instruction, state, ip, result))
            return result.proven();
        return false;
    }

    bool TypedCfgAnalyzer::transferFieldOperation(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        switch (instruction.opcode)
        {
            case OpCode::GET_FIELD: case OpCode::GET_FIELD_CACHED:
            case OpCode::GET_FIELD_TYPED: case OpCode::INLINE_GET_FIELD:
                return replaceOrFail(
                    state, 1, AbstractSlotType::BOXED, ip, result);
            case OpCode::SET_FIELD: case OpCode::SET_FIELD_CACHED:
            case OpCode::SET_FIELD_TYPED: case OpCode::INLINE_SET_FIELD:
                return replaceOrFail(
                    state, 2, AbstractSlotType::BOXED, ip, result);
            case OpCode::LOAD_GET_FIELD:
            case OpCode::LOAD_LOCAL_GET_FIELD_CACHED:
                state.stack.push_back(AbstractSlotType::BOXED);
                return true;
            default:
                return false;
        }
    }

    bool TypedCfgAnalyzer::transferObjectOperation(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        switch (instruction.opcode)
        {
            case OpCode::NEW_OBJECT: case OpCode::NEW_VALUE_OBJECT:
            case OpCode::NEW_STACK:
                return transferObjectCreation(instruction, state, ip, result);
            case OpCode::STACK_SCOPE_ENTER: case OpCode::STACK_SCOPE_LEAVE:
            case OpCode::BIND_TYPE_ARGS:
                return true;
            case OpCode::INSTANCEOF: case OpCode::INSTANCEOF_TYPEPARAM:
                return replaceOrFail(
                    state, 1, AbstractSlotType::BOOL, ip, result);
            case OpCode::CAST: case OpCode::CAST_TYPEPARAM:
            case OpCode::OBJECT_TO_VALUE: case OpCode::CREATE_PROMISE:
            case OpCode::OBJECT_TO_VALUE_CREATE_PROMISE:
            case OpCode::AWAIT: case OpCode::GET_ITERATOR:
            case OpCode::ITERATOR_NEXT:
                return replaceOrFail(
                    state, 1, AbstractSlotType::BOXED, ip, result);
            case OpCode::ITERATOR_HAS_NEXT:
                return replaceOrFail(
                    state, 1, AbstractSlotType::BOOL, ip, result);
            case OpCode::ITERATOR_CLOSE:
                return popOrFail(state, 1, ip, result);
            case OpCode::STRUCT_HASH_INT:
                return replaceOrFail(
                    state, 1, AbstractSlotType::INT, ip, result);
            case OpCode::STRUCT_EQ_INT:
                return replaceOrFail(
                    state, 2, AbstractSlotType::BOOL, ip, result);
            default:
                return false;
        }
    }

    bool TypedCfgAnalyzer::transferObjectCreation(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (!hasOperands(instruction, 2))
            return failOperands(result, ip);
        return replaceOrFail(
            state,
            static_cast<size_t>(instruction.inlineOperands[1]),
            AbstractSlotType::BOXED, ip, result);
    }

    bool TypedCfgAnalyzer::transferArrayOperation(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        switch (instruction.opcode)
        {
            case OpCode::NEW_ARRAY:
                return replaceOrFail(
                    state, 1, AbstractSlotType::BOXED, ip, result);
            case OpCode::NEW_ARRAY_MULTI:
                return transferMultiArray(instruction, state, ip, result);
            case OpCode::ARRAY_GET: case OpCode::ARRAY_GET_ALIAS:
            case OpCode::ARRAY_GET_FIELD:
                return replaceOrFail(
                    state, 2, AbstractSlotType::BOXED, ip, result);
            case OpCode::ARRAY_GET_INT:
                return replaceOrFail(
                    state, 2, AbstractSlotType::INT, ip, result);
            case OpCode::ARRAY_SET: case OpCode::ARRAY_SET_INT:
            case OpCode::ARRAY_SET_FIELD:
                return popOrFail(state, 3, ip, result);
            case OpCode::ARRAY_LENGTH:
                return replaceOrFail(
                    state, 1, AbstractSlotType::INT, ip, result);
            case OpCode::ARRAY_GET_INT_LOCAL:
                return replaceOrFail(
                    state, 1, AbstractSlotType::INT, ip, result);
            case OpCode::ARRAY_SET_INT_LOCAL:
                return popOrFail(state, 2, ip, result);
            case OpCode::ARRAY_LENGTH_LOCAL:
                state.stack.push_back(AbstractSlotType::INT);
                return true;
            default:
                return false;
        }
    }

    bool TypedCfgAnalyzer::transferMultiArray(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        if (!hasOperands(instruction, 2))
            return failOperands(result, ip);
        const size_t dimensions = instruction.numOperands() > 2
            ? static_cast<size_t>(instruction.operandAt(2))
            : static_cast<size_t>(instruction.inlineOperands[1]);
        return replaceOrFail(
            state, dimensions, AbstractSlotType::BOXED, ip, result);
    }

    bool TypedCfgAnalyzer::transferPrimitiveInvoke(
        const Instruction& instruction,
        AbstractState& state,
        size_t ip,
        FrameAnalysisResult& result)
    {
        const OpCode opcode = instruction.opcode;
        if (isPrimitiveIntBinary(opcode))
        {
            const AbstractSlotType output = isPrimitiveComparison(opcode)
                ? AbstractSlotType::BOOL
                : AbstractSlotType::INT;
            return replaceOrFail(state, 2, output, ip, result);
        }
        if (isPrimitiveFloatBinary(opcode))
        {
            const AbstractSlotType output = isPrimitiveComparison(opcode)
                ? AbstractSlotType::BOOL
                : (opcode == OpCode::INVOKE_FLOAT_COMPARE
                    ? AbstractSlotType::INT
                    : AbstractSlotType::FLOAT);
            return replaceOrFail(state, 2, output, ip, result);
        }
        if (isPrimitiveBoolBinary(opcode))
            return replaceOrFail(
                state, 2, AbstractSlotType::BOOL, ip, result);
        if (isPrimitiveUnary(opcode))
            return replaceOrFail(
                state, 1, primitiveUnaryOutput(opcode), ip, result);
        if (opcode == OpCode::INVOKE_STRING_EQUALS ||
            opcode == OpCode::INVOKE_STRING_CONCAT)
            return replaceOrFail(
                state, 2,
                opcode == OpCode::INVOKE_STRING_EQUALS
                    ? AbstractSlotType::BOOL
                    : AbstractSlotType::BOXED,
                ip, result);
        return false;
    }

    bool TypedCfgAnalyzer::isPrimitiveIntBinary(OpCode opcode) noexcept
    {
        switch (opcode)
        {
            case OpCode::INVOKE_INT_ADD: case OpCode::INVOKE_INT_SUB:
            case OpCode::INVOKE_INT_MUL: case OpCode::INVOKE_INT_DIV:
            case OpCode::INVOKE_INT_MOD: case OpCode::INVOKE_INT_EQUALS:
            case OpCode::INVOKE_INT_COMPARE:
            case OpCode::INVOKE_INT_LESS_THAN:
            case OpCode::INVOKE_INT_LESS_EQUAL:
            case OpCode::INVOKE_INT_GREATER_THAN:
            case OpCode::INVOKE_INT_GREATER_EQUAL:
                return true;
            default: return false;
        }
    }

    bool TypedCfgAnalyzer::isPrimitiveFloatBinary(OpCode opcode) noexcept
    {
        switch (opcode)
        {
            case OpCode::INVOKE_FLOAT_ADD: case OpCode::INVOKE_FLOAT_SUB:
            case OpCode::INVOKE_FLOAT_MUL: case OpCode::INVOKE_FLOAT_DIV:
            case OpCode::INVOKE_FLOAT_EQUALS:
            case OpCode::INVOKE_FLOAT_COMPARE:
            case OpCode::INVOKE_FLOAT_LESS_THAN:
            case OpCode::INVOKE_FLOAT_LESS_EQUAL:
            case OpCode::INVOKE_FLOAT_GREATER_THAN:
            case OpCode::INVOKE_FLOAT_GREATER_EQUAL:
                return true;
            default: return false;
        }
    }

    bool TypedCfgAnalyzer::isPrimitiveBoolBinary(OpCode opcode) noexcept
    {
        return opcode == OpCode::INVOKE_BOOL_AND ||
               opcode == OpCode::INVOKE_BOOL_OR ||
               opcode == OpCode::INVOKE_BOOL_XOR ||
               opcode == OpCode::INVOKE_BOOL_EQUALS;
    }

    bool TypedCfgAnalyzer::isPrimitiveComparison(OpCode opcode) noexcept
    {
        switch (opcode)
        {
            case OpCode::INVOKE_INT_EQUALS:
            case OpCode::INVOKE_INT_LESS_THAN:
            case OpCode::INVOKE_INT_LESS_EQUAL:
            case OpCode::INVOKE_INT_GREATER_THAN:
            case OpCode::INVOKE_INT_GREATER_EQUAL:
            case OpCode::INVOKE_FLOAT_EQUALS:
            case OpCode::INVOKE_FLOAT_LESS_THAN:
            case OpCode::INVOKE_FLOAT_LESS_EQUAL:
            case OpCode::INVOKE_FLOAT_GREATER_THAN:
            case OpCode::INVOKE_FLOAT_GREATER_EQUAL:
                return true;
            default: return false;
        }
    }

    bool TypedCfgAnalyzer::isPrimitiveUnary(OpCode opcode) noexcept
    {
        switch (opcode)
        {
            case OpCode::INVOKE_INT_NEG: case OpCode::INVOKE_INT_ABS:
            case OpCode::INVOKE_INT_GET_VALUE:
            case OpCode::INVOKE_FLOAT_NEG: case OpCode::INVOKE_FLOAT_ABS:
            case OpCode::INVOKE_FLOAT_GET_VALUE:
            case OpCode::INVOKE_BOOL_GET_VALUE:
            case OpCode::INVOKE_BOOL_NOT:
            case OpCode::INVOKE_STRING_LENGTH:
            case OpCode::INVOKE_STRING_IS_EMPTY:
                return true;
            default: return false;
        }
    }

    AbstractSlotType TypedCfgAnalyzer::primitiveUnaryOutput(
        OpCode opcode) noexcept
    {
        if (opcode == OpCode::INVOKE_FLOAT_NEG ||
            opcode == OpCode::INVOKE_FLOAT_ABS ||
            opcode == OpCode::INVOKE_FLOAT_GET_VALUE)
            return AbstractSlotType::FLOAT;
        if (opcode == OpCode::INVOKE_BOOL_GET_VALUE ||
            opcode == OpCode::INVOKE_BOOL_NOT ||
            opcode == OpCode::INVOKE_STRING_IS_EMPTY)
            return AbstractSlotType::BOOL;
        return AbstractSlotType::INT;
    }
}
