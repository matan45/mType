#include "AlgebraicSimplificationPattern.hpp"
#include <cstddef>
#include <cstdint>
#include "PatternSafetyHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"

namespace vm::optimization::patterns
{
    using namespace bytecode;

    bool AlgebraicSimplificationPattern::matches(const BytecodeProgram& program,
                                                  size_t offset,
                                                  const analysis::ControlFlowAnalyzer& cfg) const
    {
        return matchesAddZero(program, offset, cfg) ||
               matchesMulOne(program, offset, cfg) ||
               matchesMulZero(program, offset, cfg) ||
               matchesSubZero(program, offset, cfg);
    }

    OptimizationPattern::Replacement AlgebraicSimplificationPattern::apply(const BytecodeProgram& program,
                                                                            size_t offset) const
    {
        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);
        const auto& i3 = program.getInstruction(offset + 2);
        const auto& pool = program.getConstantPool();

        // Helper to check if instruction pushes zero
        auto isZero = [&](const BytecodeProgram::Instruction& instr, size_t instrOffset) -> bool {
            if (instr.opcode == OpCode::PUSH_INT)
            {
                return PatternSafetyHelper::safeGetInteger(pool, instr, 0, instrOffset) == 0;
            }
            if (instr.opcode == OpCode::PUSH_FLOAT)
            {
                return PatternSafetyHelper::safeGetFloat(pool, instr, 0, instrOffset) == 0.0;
            }
            return false;
        };

        // Helper to check if instruction pushes one
        auto isOne = [&](const BytecodeProgram::Instruction& instr, size_t instrOffset) -> bool {
            if (instr.opcode == OpCode::PUSH_INT)
            {
                return PatternSafetyHelper::safeGetInteger(pool, instr, 0, instrOffset) == 1;
            }
            if (instr.opcode == OpCode::PUSH_FLOAT)
            {
                return PatternSafetyHelper::safeGetFloat(pool, instr, 0, instrOffset) == 1.0;
            }
            return false;
        };

        // X + 0 or 0 + X -> X (remove PUSH_INT 0 and ADD)
        if ((i3.opcode == OpCode::ADD_INT || i3.opcode == OpCode::ADD_FLOAT) &&
            (isZero(i2, offset + 1)))
        {
            // Keep only first PUSH (X), remove second PUSH and ADD
            Replacement rep(3);
            rep.instructions.push_back(i1);
            return rep;
        }

        if ((i3.opcode == OpCode::ADD_INT || i3.opcode == OpCode::ADD_FLOAT) &&
            (isZero(i1, offset)))
        {
            // Keep only second PUSH (X), remove first PUSH and ADD
            Replacement rep(3);
            rep.instructions.push_back(i2);
            return rep;
        }

        // X * 1 or 1 * X -> X
        if ((i3.opcode == OpCode::MUL_INT || i3.opcode == OpCode::MUL_FLOAT) &&
            (isOne(i2, offset + 1)))
        {
            Replacement rep(3);
            rep.instructions.push_back(i1);
            return rep;
        }

        if ((i3.opcode == OpCode::MUL_INT || i3.opcode == OpCode::MUL_FLOAT) &&
            (isOne(i1, offset)))
        {
            Replacement rep(3);
            rep.instructions.push_back(i2);
            return rep;
        }

        // X * 0 or 0 * X -> 0
        if ((i3.opcode == OpCode::MUL_INT || i3.opcode == OpCode::MUL_FLOAT) &&
            (isZero(i2, offset + 1) || isZero(i1, offset)))
        {
            // Push 0
            auto& mutablePool = const_cast<BytecodeProgram&>(program).getConstantPool();
            size_t zeroIdx;
            if (i3.opcode == OpCode::MUL_INT)
            {
                zeroIdx = mutablePool.addInteger(0);
            }
            else
            {
                zeroIdx = mutablePool.addFloat(0.0);
            }

            Replacement rep(3);
            rep.instructions.push_back(BytecodeProgram::Instruction(
                i3.opcode == OpCode::MUL_INT ? OpCode::PUSH_INT : OpCode::PUSH_FLOAT,
                static_cast<uint64_t>(zeroIdx)
            ));
            return rep;
        }

        // X - 0 -> X
        if ((i3.opcode == OpCode::SUB_INT || i3.opcode == OpCode::SUB_FLOAT) &&
            (isZero(i2, offset + 1)))
        {
            Replacement rep(3);
            rep.instructions.push_back(i1);
            return rep;
        }

        return Replacement(0);
    }

    bool AlgebraicSimplificationPattern::matchesAddZero(const BytecodeProgram& program,
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

        if ((i3.opcode != OpCode::ADD_INT && i3.opcode != OpCode::ADD_FLOAT) ||
            (i1.opcode != OpCode::PUSH_INT && i1.opcode != OpCode::PUSH_FLOAT) ||
            (i2.opcode != OpCode::PUSH_INT && i2.opcode != OpCode::PUSH_FLOAT))
        {
            return false;
        }

        const auto& pool = program.getConstantPool();

        // Check if either operand is zero
        bool firstIsZero = false;
        bool secondIsZero = false;

        if (i1.opcode == OpCode::PUSH_INT)
        {
            firstIsZero = (PatternSafetyHelper::safeGetInteger(pool, i1, 0, offset) == 0);
        }
        else if (i1.opcode == OpCode::PUSH_FLOAT)
        {
            firstIsZero = (PatternSafetyHelper::safeGetFloat(pool, i1, 0, offset) == 0.0);
        }

        if (i2.opcode == OpCode::PUSH_INT)
        {
            secondIsZero = (PatternSafetyHelper::safeGetInteger(pool, i2, 0, offset + 1) == 0);
        }
        else if (i2.opcode == OpCode::PUSH_FLOAT)
        {
            secondIsZero = (PatternSafetyHelper::safeGetFloat(pool, i2, 0, offset + 1) == 0.0);
        }

        return (firstIsZero || secondIsZero) && cfg.canOptimizeRange(offset, offset + 3);
    }

    bool AlgebraicSimplificationPattern::matchesMulOne(const BytecodeProgram& program,
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

        if ((i3.opcode != OpCode::MUL_INT && i3.opcode != OpCode::MUL_FLOAT) ||
            (i1.opcode != OpCode::PUSH_INT && i1.opcode != OpCode::PUSH_FLOAT) ||
            (i2.opcode != OpCode::PUSH_INT && i2.opcode != OpCode::PUSH_FLOAT))
        {
            return false;
        }

        const auto& pool = program.getConstantPool();

        bool firstIsOne = false;
        bool secondIsOne = false;

        if (i1.opcode == OpCode::PUSH_INT)
        {
            firstIsOne = (PatternSafetyHelper::safeGetInteger(pool, i1, 0, offset) == 1);
        }
        else if (i1.opcode == OpCode::PUSH_FLOAT)
        {
            firstIsOne = (PatternSafetyHelper::safeGetFloat(pool, i1, 0, offset) == 1.0);
        }

        if (i2.opcode == OpCode::PUSH_INT)
        {
            secondIsOne = (PatternSafetyHelper::safeGetInteger(pool, i2, 0, offset + 1) == 1);
        }
        else if (i2.opcode == OpCode::PUSH_FLOAT)
        {
            secondIsOne = (PatternSafetyHelper::safeGetFloat(pool, i2, 0, offset + 1) == 1.0);
        }

        return (firstIsOne || secondIsOne) && cfg.canOptimizeRange(offset, offset + 3);
    }

    bool AlgebraicSimplificationPattern::matchesMulZero(const BytecodeProgram& program,
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

        if ((i3.opcode != OpCode::MUL_INT && i3.opcode != OpCode::MUL_FLOAT) ||
            (i1.opcode != OpCode::PUSH_INT && i1.opcode != OpCode::PUSH_FLOAT) ||
            (i2.opcode != OpCode::PUSH_INT && i2.opcode != OpCode::PUSH_FLOAT))
        {
            return false;
        }

        const auto& pool = program.getConstantPool();

        bool firstIsZero = false;
        bool secondIsZero = false;

        if (i1.opcode == OpCode::PUSH_INT)
        {
            firstIsZero = (PatternSafetyHelper::safeGetInteger(pool, i1, 0, offset) == 0);
        }
        else if (i1.opcode == OpCode::PUSH_FLOAT)
        {
            firstIsZero = (PatternSafetyHelper::safeGetFloat(pool, i1, 0, offset) == 0.0);
        }

        if (i2.opcode == OpCode::PUSH_INT)
        {
            secondIsZero = (PatternSafetyHelper::safeGetInteger(pool, i2, 0, offset + 1) == 0);
        }
        else if (i2.opcode == OpCode::PUSH_FLOAT)
        {
            secondIsZero = (PatternSafetyHelper::safeGetFloat(pool, i2, 0, offset + 1) == 0.0);
        }

        return (firstIsZero || secondIsZero) && cfg.canOptimizeRange(offset, offset + 3);
    }

    bool AlgebraicSimplificationPattern::matchesSubZero(const BytecodeProgram& program,
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

        if ((i3.opcode != OpCode::SUB_INT && i3.opcode != OpCode::SUB_FLOAT) ||
            (i1.opcode != OpCode::PUSH_INT && i1.opcode != OpCode::PUSH_FLOAT) ||
            (i2.opcode != OpCode::PUSH_INT && i2.opcode != OpCode::PUSH_FLOAT))
        {
            return false;
        }

        const auto& pool = program.getConstantPool();

        bool secondIsZero = false;

        if (i2.opcode == OpCode::PUSH_INT)
        {
            secondIsZero = (PatternSafetyHelper::safeGetInteger(pool, i2, 0, offset + 1) == 0);
        }
        else if (i2.opcode == OpCode::PUSH_FLOAT)
        {
            secondIsZero = (PatternSafetyHelper::safeGetFloat(pool, i2, 0, offset + 1) == 0.0);
        }

        return secondIsZero && cfg.canOptimizeRange(offset, offset + 3);
    }

} // namespace vm::optimization::patterns
