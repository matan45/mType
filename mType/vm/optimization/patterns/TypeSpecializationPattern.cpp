#include "TypeSpecializationPattern.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"

namespace vm::optimization::patterns
{
    using namespace bytecode;

    bool TypeSpecializationPattern::matches(const BytecodeProgram& program,
                                            size_t offset,
                                            const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 2 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        // Check if both operands are the same type constant pushes
        bool bothInt = (i1.opcode == OpCode::PUSH_INT && i2.opcode == OpCode::PUSH_INT);
        bool bothFloat = (i1.opcode == OpCode::PUSH_FLOAT && i2.opcode == OpCode::PUSH_FLOAT);

        if (!bothInt && !bothFloat)
        {
            return false;
        }

        // Check if operation is generic (can be specialized)
        // NOTE: Comparison specialization is disabled because VM doesn't implement specialized comparison opcodes yet
        // Only arithmetic operations (ADD, SUB, MUL, DIV) are specialized
        if (!isGenericArithmetic(i3.opcode))
        {
            return false;
        }

        // Type specialization with constant operands is always safe
        // Even at basic block boundaries, because constants have no control flow dependencies
        return true;
    }

    OptimizationPattern::Replacement TypeSpecializationPattern::apply(const BytecodeProgram& program,
                                                                       size_t offset) const
    {
        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);

        // Get the specialized operation
        OpCode specializedOp = getSpecializedOpCode(i3.opcode, i1.opcode);

        if (specializedOp == i3.opcode)
        {
            // No specialization available
            return Replacement(0);
        }

        // Replace generic operation with specialized one
        Replacement rep(3);
        rep.instructions.push_back(i1);
        rep.instructions.push_back(i2);
        rep.instructions.push_back(BytecodeProgram::Instruction(specializedOp));

        return rep;
    }

    OpCode TypeSpecializationPattern::getSpecializedOpCode(OpCode genericOp,
                                                           OpCode pushType) const
    {
        bool isInt = (pushType == OpCode::PUSH_INT);
        bool isFloat = (pushType == OpCode::PUSH_FLOAT);

        // Arithmetic operations
        if (genericOp == OpCode::ADD)
        {
            return isInt ? OpCode::ADD_INT : (isFloat ? OpCode::ADD_FLOAT : genericOp);
        }
        if (genericOp == OpCode::SUB)
        {
            return isInt ? OpCode::SUB_INT : (isFloat ? OpCode::SUB_FLOAT : genericOp);
        }
        if (genericOp == OpCode::MUL)
        {
            return isInt ? OpCode::MUL_INT : (isFloat ? OpCode::MUL_FLOAT : genericOp);
        }
        if (genericOp == OpCode::DIV)
        {
            return isInt ? OpCode::DIV_INT : (isFloat ? OpCode::DIV_FLOAT : genericOp);
        }

        // Comparison operations (only for integers)
        if (isInt)
        {
            if (genericOp == OpCode::EQ) return OpCode::EQ_INT;
            if (genericOp == OpCode::NE) return OpCode::NE_INT;
            if (genericOp == OpCode::LT) return OpCode::LT_INT;
            if (genericOp == OpCode::GT) return OpCode::GT_INT;
        }

        return genericOp;  // No specialization available
    }

    bool TypeSpecializationPattern::isGenericArithmetic(OpCode opcode) const
    {
        return opcode == OpCode::ADD ||
               opcode == OpCode::SUB ||
               opcode == OpCode::MUL ||
               opcode == OpCode::DIV ||
               opcode == OpCode::MOD;
    }

    bool TypeSpecializationPattern::isGenericComparison(OpCode opcode) const
    {
        return opcode == OpCode::EQ ||
               opcode == OpCode::NE ||
               opcode == OpCode::LT ||
               opcode == OpCode::GT ||
               opcode == OpCode::LE ||
               opcode == OpCode::GE;
    }

} // namespace vm::optimization::patterns
