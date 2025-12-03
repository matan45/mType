#include "StrengthReductionPattern.hpp"
#include "PatternSafetyHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"

namespace vm::optimization::patterns
{
    using namespace bytecode;

    bool StrengthReductionPattern::matches(const BytecodeProgram& program,
                                           size_t offset,
                                           const analysis::ControlFlowAnalyzer& cfg) const
    {
        return matchesDivOne(program, offset, cfg) ||
               matchesDivNegOne(program, offset, cfg);
    }

    OptimizationPattern::Replacement StrengthReductionPattern::apply(const BytecodeProgram& program,
                                                                      size_t offset) const
    {
        if (offset + 2 >= program.getInstructionCount())
        {
            return Replacement(0);
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);
        const auto& pool = program.getConstantPool();

        // X / 1 → X
        if ((i1.opcode == OpCode::PUSH_INT || i1.opcode == OpCode::PUSH_FLOAT) &&
            (i2.opcode == OpCode::PUSH_INT || i2.opcode == OpCode::PUSH_FLOAT) &&
            (i3.opcode == OpCode::DIV_INT || i3.opcode == OpCode::DIV_FLOAT))
        {
            // Check if second operand is 1
            if (i2.opcode == OpCode::PUSH_INT)
            {
                int val2 = static_cast<int>(PatternSafetyHelper::safeGetInteger(pool, i2, 0, offset + 1));
                if (val2 == 1 && i3.opcode == OpCode::DIV_INT)
                {
                    // Keep only first operand
                    Replacement rep(3);
                    rep.instructions.push_back(i1);
                    return rep;
                }

                // X / -1 → -X
                if (val2 == -1 && i3.opcode == OpCode::DIV_INT)
                {
                    // Replace with: X, NEG
                    Replacement rep(3);
                    rep.instructions.push_back(i1);
                    rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::NEG));
                    return rep;
                }
            }
            else if (i2.opcode == OpCode::PUSH_FLOAT)
            {
                double fval2 = PatternSafetyHelper::safeGetFloat(pool, i2, 0, offset + 1);
                if (fval2 == 1.0 && i3.opcode == OpCode::DIV_FLOAT)
                {
                    // Keep only first operand
                    Replacement rep(3);
                    rep.instructions.push_back(i1);
                    return rep;
                }
            }
        }

        return Replacement(0);
    }

    bool StrengthReductionPattern::matchesDivOne(const BytecodeProgram& program,
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

        // Match: PUSH, PUSH 1, DIV
        if ((i1.opcode != OpCode::PUSH_INT && i1.opcode != OpCode::PUSH_FLOAT) ||
            (i2.opcode != OpCode::PUSH_INT && i2.opcode != OpCode::PUSH_FLOAT))
        {
            return false;
        }

        if (i3.opcode != OpCode::DIV_INT && i3.opcode != OpCode::DIV_FLOAT)
        {
            return false;
        }

        const auto& pool = program.getConstantPool();

        // Check if second operand is 1
        if (i2.opcode == OpCode::PUSH_INT)
        {
            int val = static_cast<int>(PatternSafetyHelper::safeGetInteger(pool, i2, 0, offset + 1));
            if (val == 1)
            {
                return cfg.canOptimizeRange(offset, offset + 3);
            }
        }
        else if (i2.opcode == OpCode::PUSH_FLOAT)
        {
            double val = PatternSafetyHelper::safeGetFloat(pool, i2, 0, offset + 1);
            if (val == 1.0)
            {
                return cfg.canOptimizeRange(offset, offset + 3);
            }
        }

        return false;
    }

    bool StrengthReductionPattern::matchesDivNegOne(const BytecodeProgram& program,
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

        // Match: PUSH_INT, PUSH_INT -1, DIV_INT
        if (i1.opcode != OpCode::PUSH_INT || i2.opcode != OpCode::PUSH_INT)
        {
            return false;
        }

        if (i3.opcode != OpCode::DIV_INT)
        {
            return false;
        }

        const auto& pool = program.getConstantPool();
        int val = static_cast<int>(PatternSafetyHelper::safeGetInteger(pool, i2, 0, offset + 1));

        if (val == -1)
        {
            return cfg.canOptimizeRange(offset, offset + 3);
        }

        return false;
    }

    bool StrengthReductionPattern::isPowerOfTwo(int value)
    {
        return value > 0 && (value & (value - 1)) == 0;
    }

} // namespace vm::optimization::patterns
