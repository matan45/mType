#include "StackOptimizationPattern.hpp"
#include <cstddef>
#include "../../bytecode/OpCode.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"

namespace vm::optimization::patterns
{
    using namespace bytecode;

    bool StackOptimizationPattern::matches(const BytecodeProgram& program,
                                           size_t offset,
                                           const analysis::ControlFlowAnalyzer& cfg) const
    {
        return matchesDoubleSwap(program, offset, cfg) ||
               matchesDupPop(program, offset, cfg) ||
               matchesDoubleLoad(program, offset, cfg);
    }

    OptimizationPattern::Replacement StackOptimizationPattern::apply(const BytecodeProgram& program,
                                                                      size_t offset) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return Replacement(0);
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // SWAP, SWAP → (removed)
        if (i1.opcode == OpCode::SWAP && i2.opcode == OpCode::SWAP)
        {
            return Replacement(2);  // Remove both SWAP instructions
        }

        // DUP, POP → (removed)
        if (i1.opcode == OpCode::DUP && i2.opcode == OpCode::POP)
        {
            return Replacement(2);  // Remove both instructions
        }

        // LOAD_LOCAL X, LOAD_LOCAL X → LOAD X, DUP
        if (i1.opcode == OpCode::LOAD_LOCAL && i2.opcode == OpCode::LOAD_LOCAL)
        {
            if (!i1.operands.empty() && !i2.operands.empty() &&
                i1.operands[0] == i2.operands[0])
            {
                Replacement rep(2);
                rep.instructions.push_back(i1);  // Keep first LOAD
                rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::DUP));  // Replace second with DUP
                return rep;
            }
        }

        // LOAD_GLOBAL X, LOAD_GLOBAL X → LOAD X, DUP
        if (i1.opcode == OpCode::LOAD_GLOBAL && i2.opcode == OpCode::LOAD_GLOBAL)
        {
            if (!i1.operands.empty() && !i2.operands.empty() &&
                i1.operands[0] == i2.operands[0])
            {
                Replacement rep(2);
                rep.instructions.push_back(i1);  // Keep first LOAD
                rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::DUP));  // Replace second with DUP
                return rep;
            }
        }

        return Replacement(0);
    }

    bool StackOptimizationPattern::matchesDoubleSwap(const BytecodeProgram& program,
                                                      size_t offset,
                                                      const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // Match: SWAP, SWAP
        if (i1.opcode == OpCode::SWAP && i2.opcode == OpCode::SWAP)
        {
            return cfg.canOptimizeRange(offset, offset + 2);
        }

        return false;
    }

    bool StackOptimizationPattern::matchesDupPop(const BytecodeProgram& program,
                                                  size_t offset,
                                                  const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // Match: DUP, POP
        if (i1.opcode == OpCode::DUP && i2.opcode == OpCode::POP)
        {
            return cfg.canOptimizeRange(offset, offset + 2);
        }

        return false;
    }

    bool StackOptimizationPattern::matchesDoubleLoad(const BytecodeProgram& program,
                                                      size_t offset,
                                                      const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // Match: LOAD_LOCAL X, LOAD_LOCAL X (same variable)
        if (i1.opcode == OpCode::LOAD_LOCAL && i2.opcode == OpCode::LOAD_LOCAL)
        {
            if (!i1.operands.empty() && !i2.operands.empty() &&
                i1.operands[0] == i2.operands[0])
            {
                return cfg.canOptimizeRange(offset, offset + 2);
            }
        }

        // Match: LOAD_GLOBAL X, LOAD_GLOBAL X (same variable)
        if (i1.opcode == OpCode::LOAD_GLOBAL && i2.opcode == OpCode::LOAD_GLOBAL)
        {
            if (!i1.operands.empty() && !i2.operands.empty() &&
                i1.operands[0] == i2.operands[0])
            {
                return cfg.canOptimizeRange(offset, offset + 2);
            }
        }

        return false;
    }

} // namespace vm::optimization::patterns
