#include "OptimizationPattern.hpp"
#include "../bytecode/OpCode.hpp"

namespace vm::optimization
{
    using namespace bytecode;

    bool OptimizationPattern::isConstantPush(OpCode opcode)
    {
        return opcode == OpCode::PUSH_INT ||
               opcode == OpCode::PUSH_FLOAT ||
               opcode == OpCode::PUSH_STRING ||
               opcode == OpCode::PUSH_BOOL ||
               opcode == OpCode::PUSH_NULL;
    }

    bool OptimizationPattern::isJumpInstruction(OpCode opcode)
    {
        return opcode == OpCode::JUMP ||
               opcode == OpCode::JUMP_IF_FALSE ||
               opcode == OpCode::JUMP_IF_TRUE ||
               opcode == OpCode::JUMP_IF_NULL ||
               opcode == OpCode::JUMP_BACK ||
               opcode == OpCode::JUMP_IF_FALSE_OR_POP ||
               opcode == OpCode::JUMP_IF_TRUE_OR_POP;
    }

    bool OptimizationPattern::isBinaryArithmetic(OpCode opcode)
    {
        return opcode == OpCode::ADD ||
               opcode == OpCode::SUB ||
               opcode == OpCode::MUL ||
               opcode == OpCode::DIV ||
               opcode == OpCode::MOD ||
               opcode == OpCode::ADD_INT ||
               opcode == OpCode::SUB_INT ||
               opcode == OpCode::MUL_INT ||
               opcode == OpCode::DIV_INT ||
               opcode == OpCode::ADD_FLOAT ||
               opcode == OpCode::SUB_FLOAT ||
               opcode == OpCode::MUL_FLOAT ||
               opcode == OpCode::DIV_FLOAT;
    }

    bool OptimizationPattern::isUnaryOperation(OpCode opcode)
    {
        return opcode == OpCode::NEG ||
               opcode == OpCode::NOT ||
               opcode == OpCode::INC ||
               opcode == OpCode::DEC;
    }

    bool OptimizationPattern::isComparisonOperation(OpCode opcode)
    {
        return opcode == OpCode::EQ ||
               opcode == OpCode::NE ||
               opcode == OpCode::LT ||
               opcode == OpCode::GT ||
               opcode == OpCode::LE ||
               opcode == OpCode::GE ||
               opcode == OpCode::EQ_INT ||
               opcode == OpCode::NE_INT ||
               opcode == OpCode::LT_INT ||
               opcode == OpCode::GT_INT;
    }

    bool OptimizationPattern::modifiesControlFlow(OpCode opcode)
    {
        return isJumpInstruction(opcode) ||
               opcode == OpCode::RETURN ||
               opcode == OpCode::RETURN_VALUE ||
               opcode == OpCode::THROW ||
               opcode == OpCode::CALL ||
               opcode == OpCode::CALL_FAST ||
               opcode == OpCode::TAIL_CALL;
    }

    bool OptimizationPattern::hasSideEffects(OpCode opcode)
    {
        // Instructions that modify memory or call functions
        return opcode == OpCode::STORE_VAR ||
               opcode == OpCode::STORE_LOCAL ||
               opcode == OpCode::STORE_GLOBAL ||
               opcode == OpCode::STORE_UPVALUE ||
               opcode == OpCode::SET_FIELD ||
               opcode == OpCode::SET_FIELD_FAST ||
               opcode == OpCode::SET_STATIC ||
               opcode == OpCode::SUPER_SET_FIELD ||
               opcode == OpCode::ARRAY_SET ||
               opcode == OpCode::ARRAY_SET_INT ||
               opcode == OpCode::ARRAY_SET_FIELD ||
               opcode == OpCode::CALL ||
               opcode == OpCode::CALL_METHOD ||
               opcode == OpCode::CALL_STATIC ||
               opcode == OpCode::INVOKE ||
               opcode == OpCode::SUPER_INVOKE ||
               opcode == OpCode::THROW ||
               opcode == OpCode::NEW_OBJECT ||
               opcode == OpCode::NEW_ARRAY ||
               opcode == OpCode::NEW_ARRAY_MULTI;
    }

    int OptimizationPattern::getStackConsumption(OpCode opcode)
    {
        switch (opcode)
        {
            // Binary operations consume 2
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
            case OpCode::MOD:
            case OpCode::ADD_INT:
            case OpCode::SUB_INT:
            case OpCode::MUL_INT:
            case OpCode::DIV_INT:
            case OpCode::ADD_FLOAT:
            case OpCode::SUB_FLOAT:
            case OpCode::MUL_FLOAT:
            case OpCode::DIV_FLOAT:
            case OpCode::AND:
            case OpCode::OR:
            case OpCode::XOR:
            case OpCode::EQ:
            case OpCode::NE:
            case OpCode::LT:
            case OpCode::GT:
            case OpCode::LE:
            case OpCode::GE:
            case OpCode::EQ_INT:
            case OpCode::NE_INT:
            case OpCode::LT_INT:
            case OpCode::GT_INT:
            case OpCode::ARRAY_GET:
            case OpCode::ARRAY_GET_INT:
            case OpCode::SWAP:
                return 2;

            // Unary operations consume 1
            case OpCode::NEG:
            case OpCode::NOT:
            case OpCode::POP:
            case OpCode::RETURN_VALUE:
            case OpCode::JUMP_IF_FALSE:
            case OpCode::JUMP_IF_TRUE:
            case OpCode::JUMP_IF_NULL:
            case OpCode::INC:
            case OpCode::DEC:
            case OpCode::STORE_VAR:
            case OpCode::STORE_LOCAL:
            case OpCode::STORE_GLOBAL:
            case OpCode::INSTANCEOF:
            case OpCode::CAST:
            case OpCode::CHECK_TYPE:
            case OpCode::ARRAY_LENGTH:
                return 1;

            // Push operations consume 0
            case OpCode::PUSH_INT:
            case OpCode::PUSH_FLOAT:
            case OpCode::PUSH_STRING:
            case OpCode::PUSH_BOOL:
            case OpCode::PUSH_NULL:
            case OpCode::LOAD_VAR:
            case OpCode::LOAD_LOCAL:
            case OpCode::LOAD_GLOBAL:
            case OpCode::DUP:
                return 0;

            // DUP2 consumes 0 (copies top 2 elements)
            case OpCode::DUP2:
                return 0;

            default:
                // Conservative default
                return 0;
        }
    }

    int OptimizationPattern::getStackProduction(OpCode opcode)
    {
        switch (opcode)
        {
            // Binary operations produce 1 (consume 2, produce 1)
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
            case OpCode::MOD:
            case OpCode::ADD_INT:
            case OpCode::SUB_INT:
            case OpCode::MUL_INT:
            case OpCode::DIV_INT:
            case OpCode::ADD_FLOAT:
            case OpCode::SUB_FLOAT:
            case OpCode::MUL_FLOAT:
            case OpCode::DIV_FLOAT:
            case OpCode::AND:
            case OpCode::OR:
            case OpCode::XOR:
            case OpCode::EQ:
            case OpCode::NE:
            case OpCode::LT:
            case OpCode::GT:
            case OpCode::LE:
            case OpCode::GE:
            case OpCode::EQ_INT:
            case OpCode::NE_INT:
            case OpCode::LT_INT:
            case OpCode::GT_INT:
            case OpCode::ARRAY_GET:
            case OpCode::ARRAY_GET_INT:
            case OpCode::NEG:
            case OpCode::NOT:
            case OpCode::INC:
            case OpCode::DEC:
            case OpCode::INSTANCEOF:
            case OpCode::CAST:
            case OpCode::CHECK_TYPE:
            case OpCode::ARRAY_LENGTH:
                return 1;

            // Push operations produce 1
            case OpCode::PUSH_INT:
            case OpCode::PUSH_FLOAT:
            case OpCode::PUSH_STRING:
            case OpCode::PUSH_BOOL:
            case OpCode::PUSH_NULL:
            case OpCode::LOAD_VAR:
            case OpCode::LOAD_LOCAL:
            case OpCode::LOAD_GLOBAL:
                return 1;

            // DUP produces 1 additional (1 element becomes 2)
            case OpCode::DUP:
                return 1;

            // DUP2 produces 2 additional (top 2 elements become 4)
            case OpCode::DUP2:
                return 2;

            // SWAP produces 0 net (just reorders)
            case OpCode::SWAP:
                return 0;

            // POP, STORE, RETURN_VALUE produce 0
            case OpCode::POP:
            case OpCode::STORE_VAR:
            case OpCode::STORE_LOCAL:
            case OpCode::STORE_GLOBAL:
            case OpCode::RETURN_VALUE:
            case OpCode::JUMP_IF_FALSE:
            case OpCode::JUMP_IF_TRUE:
            case OpCode::JUMP_IF_NULL:
                return 0;

            // NOP, JUMP produce 0
            case OpCode::NOP:
            case OpCode::JUMP:
            case OpCode::JUMP_BACK:
                return 0;

            default:
                // Conservative default
                return 0;
        }
    }

} // namespace vm::optimization
