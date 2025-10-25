#include "DeadCodePattern.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"

namespace vm::optimization::patterns
{
    using namespace bytecode;

    bool DeadCodePattern::matches(const BytecodeProgram& program,
                                  size_t offset,
                                  const analysis::ControlFlowAnalyzer& cfg) const
    {
        return matchesPushPop(program, offset, cfg) ||
               matchesNOP(program, offset);
    }

    OptimizationPattern::Replacement DeadCodePattern::apply(const BytecodeProgram& program,
                                                            size_t offset) const
    {
        // Check NOP first (doesn't need CFG)
        if (matchesNOP(program, offset))
        {
            return Replacement(1);
        }

        // Check PUSH+POP (doesn't need CFG for detection, just pattern matching)
        if (offset + 1 < program.getInstructionCount())
        {
            const auto& i1 = program.getInstruction(offset);
            const auto& i2 = program.getInstruction(offset + 1);

            bool isPush = isConstantPush(i1.opcode) ||
                         i1.opcode == OpCode::LOAD_VAR ||
                         i1.opcode == OpCode::LOAD_LOCAL ||
                         i1.opcode == OpCode::LOAD_GLOBAL;

            if (isPush && i2.opcode == OpCode::POP)
            {
                return Replacement(2);
            }
        }

        // Should not reach here - matches() should have validated the pattern
        return Replacement(0);
    }

    bool DeadCodePattern::matchesPushPop(const BytecodeProgram& program,
                                         size_t offset,
                                         const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // Match: <PUSH>, POP
        bool isPush = isConstantPush(i1.opcode) ||
                     i1.opcode == OpCode::LOAD_VAR ||
                     i1.opcode == OpCode::LOAD_LOCAL ||
                     i1.opcode == OpCode::LOAD_GLOBAL;

        if (!isPush || i2.opcode != OpCode::POP)
        {
            return false;
        }

        // Ensure no jump targets in between
        return cfg.canOptimizeRange(offset, offset + 2);
    }

    bool DeadCodePattern::matchesNOP(const bytecode::BytecodeProgram& program,
                                     size_t offset) const
    {
        if (offset >= program.getInstructionCount())
        {
            return false;
        }

        const auto& instr = program.getInstruction(offset);
        return instr.opcode == OpCode::NOP;
    }

} // namespace vm::optimization::patterns
