#include "NullSequencePattern.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"

namespace vm::optimization::patterns
{
    using namespace bytecode;

    bool NullSequencePattern::matches(const BytecodeProgram& program,
                                      size_t offset,
                                      const analysis::ControlFlowAnalyzer& cfg) const
    {
        return matchesDoubleNeg(program, offset, cfg) ||
               matchesDoubleNot(program, offset, cfg);
    }

    OptimizationPattern::Replacement NullSequencePattern::apply(const BytecodeProgram& program,
                                                                 size_t offset) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return Replacement(0);
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // NEG, NEG → (removed)
        if (i1.opcode == OpCode::NEG && i2.opcode == OpCode::NEG)
        {
            return Replacement(2);  // Remove both instructions
        }

        // NOT, NOT → (removed)
        if (i1.opcode == OpCode::NOT && i2.opcode == OpCode::NOT)
        {
            return Replacement(2);  // Remove both instructions
        }

        return Replacement(0);
    }

    bool NullSequencePattern::matchesDoubleNeg(const BytecodeProgram& program,
                                                size_t offset,
                                                const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // Match: NEG, NEG
        if (i1.opcode == OpCode::NEG && i2.opcode == OpCode::NEG)
        {
            return cfg.canOptimizeRange(offset, offset + 2);
        }

        return false;
    }

    bool NullSequencePattern::matchesDoubleNot(const BytecodeProgram& program,
                                                size_t offset,
                                                const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // Match: NOT, NOT
        if (i1.opcode == OpCode::NOT && i2.opcode == OpCode::NOT)
        {
            return cfg.canOptimizeRange(offset, offset + 2);
        }

        return false;
    }

} // namespace vm::optimization::patterns
