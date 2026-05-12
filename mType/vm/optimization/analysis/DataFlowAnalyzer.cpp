#include "DataFlowAnalyzer.hpp"
#include <cstddef>
#include "../../bytecode/OpCode.hpp"

namespace vm::optimization::analysis
{
    using namespace bytecode;

    void DataFlowAnalyzer::analyze(const BytecodeProgram& program)
    {
        clear();

        const auto& instructions = program.getInstructions();

        for (size_t i = 0; i < instructions.size(); ++i)
        {
            const auto& instr = instructions[i];

            // Calculate and store stack effect
            stackEffects[i] = calculateStackEffect(instr.opcode);

            // Track constant-producing instructions
            if (instr.opcode == OpCode::PUSH_INT ||
                instr.opcode == OpCode::PUSH_FLOAT ||
                instr.opcode == OpCode::PUSH_STRING ||
                instr.opcode == OpCode::PUSH_BOOL ||
                instr.opcode == OpCode::PUSH_NULL)
            {
                constantProducers[i] = true;
            }
        }
    }

    DataFlowAnalyzer::StackEffect DataFlowAnalyzer::getStackEffect(size_t offset) const
    {
        auto it = stackEffects.find(offset);
        if (it != stackEffects.end())
        {
            return it->second;
        }
        return StackEffect(0, 0);
    }

    DataFlowAnalyzer::StackEffect DataFlowAnalyzer::calculateStackEffect(OpCode opcode)
    {
        switch (opcode)
        {
            // Binary operations: consume 2, produce 1
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
                return StackEffect(2, 1);

            // Unary operations: consume 1, produce 1
            case OpCode::NEG:
            case OpCode::NOT:
            case OpCode::INC:
            case OpCode::DEC:
                return StackEffect(1, 1);

            // Push operations: consume 0, produce 1
            case OpCode::PUSH_INT:
            case OpCode::PUSH_FLOAT:
            case OpCode::PUSH_STRING:
            case OpCode::PUSH_BOOL:
            case OpCode::PUSH_NULL:
            case OpCode::LOAD_VAR:
            case OpCode::LOAD_LOCAL:
            case OpCode::LOAD_GLOBAL:
                return StackEffect(0, 1);

            // Pop operations: consume 1, produce 0
            case OpCode::POP:
            case OpCode::STORE_VAR:
            case OpCode::STORE_LOCAL:
            case OpCode::STORE_GLOBAL:
                return StackEffect(1, 0);

            // DUP: consume 0, produce 1 (duplicates top)
            case OpCode::DUP:
                return StackEffect(0, 1);

            // DUP2: consume 0, produce 2 (duplicates top 2)
            case OpCode::DUP2:
                return StackEffect(0, 2);

            // SWAP: consume 0, produce 0 (just reorders)
            case OpCode::SWAP:
                return StackEffect(0, 0);

            // Array operations
            case OpCode::ARRAY_GET:
            case OpCode::ARRAY_GET_ALIAS:
            case OpCode::ARRAY_GET_INT:
                return StackEffect(2, 1);  // array + index -> value

            case OpCode::ARRAY_SET:
            case OpCode::ARRAY_SET_INT:
                return StackEffect(3, 0);  // array + index + value -> nothing

            case OpCode::ARRAY_LENGTH:
                return StackEffect(1, 1);  // array -> length

            // Return operations
            case OpCode::RETURN_VALUE:
                return StackEffect(1, 0);

            case OpCode::RETURN:
                return StackEffect(0, 0);

            // Jump operations (conditional)
            case OpCode::JUMP_IF_FALSE:
            case OpCode::JUMP_IF_TRUE:
            case OpCode::JUMP_IF_NULL:
                return StackEffect(1, 0);

            case OpCode::JUMP_IF_FALSE_OR_POP:
            case OpCode::JUMP_IF_TRUE_OR_POP:
                return StackEffect(1, 0);  // Conditionally pops

            // No-ops
            case OpCode::NOP:
            case OpCode::JUMP:
            case OpCode::JUMP_BACK:
            case OpCode::LINE:
            case OpCode::SOURCE_FILE:
            case OpCode::LOOP_START:
            case OpCode::LOOP_END:
                return StackEffect(0, 0);

            default:
                // Conservative default: unknown effect
                return StackEffect(0, 0);
        }
    }

    bool DataFlowAnalyzer::producesConstant(size_t offset) const
    {
        auto it = constantProducers.find(offset);
        return it != constantProducers.end() && it->second;
    }

    void DataFlowAnalyzer::clear()
    {
        stackEffects.clear();
        constantProducers.clear();
    }

} // namespace vm::optimization::analysis
