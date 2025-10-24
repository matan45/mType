#include "LoadStoreOptimizationPattern.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"

namespace vm::optimization::patterns
{
    using namespace bytecode;

    bool LoadStoreOptimizationPattern::matches(const BytecodeProgram& program,
                                               size_t offset,
                                               const analysis::ControlFlowAnalyzer& cfg) const
    {
        return matchesStoreLoadLocal(program, offset, cfg) ||
               matchesStoreLoadGlobal(program, offset, cfg);
    }

    OptimizationPattern::Replacement LoadStoreOptimizationPattern::apply(const BytecodeProgram& program,
                                                                          size_t offset) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return Replacement(0);
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // STORE_LOCAL X, LOAD_LOCAL X → STORE_LOCAL X, DUP
        if (i1.opcode == OpCode::STORE_LOCAL && i2.opcode == OpCode::LOAD_LOCAL)
        {
            if (!i1.operands.empty() && !i2.operands.empty() &&
                i1.operands[0] == i2.operands[0])
            {
                Replacement rep(2);
                rep.instructions.push_back(i1);  // Keep STORE
                rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::DUP));  // Replace LOAD with DUP
                return rep;
            }
        }

        // STORE_GLOBAL X, LOAD_GLOBAL X → STORE_GLOBAL X, DUP
        if (i1.opcode == OpCode::STORE_GLOBAL && i2.opcode == OpCode::LOAD_GLOBAL)
        {
            if (!i1.operands.empty() && !i2.operands.empty() &&
                i1.operands[0] == i2.operands[0])
            {
                Replacement rep(2);
                rep.instructions.push_back(i1);  // Keep STORE
                rep.instructions.push_back(BytecodeProgram::Instruction(OpCode::DUP));  // Replace LOAD with DUP
                return rep;
            }
        }

        return Replacement(0);
    }

    bool LoadStoreOptimizationPattern::matchesStoreLoadLocal(const BytecodeProgram& program,
                                                              size_t offset,
                                                              const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // Match: STORE_LOCAL X, LOAD_LOCAL X (same slot)
        if (i1.opcode == OpCode::STORE_LOCAL && i2.opcode == OpCode::LOAD_LOCAL)
        {
            if (!i1.operands.empty() && !i2.operands.empty() &&
                i1.operands[0] == i2.operands[0])
            {
                return cfg.canOptimizeRange(offset, offset + 2);
            }
        }

        return false;
    }

    bool LoadStoreOptimizationPattern::matchesStoreLoadGlobal(const BytecodeProgram& program,
                                                               size_t offset,
                                                               const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset + 1 >= program.getInstructionCount())
        {
            return false;
        }

        const auto& i1 = program.getInstruction(offset);
        const auto& i2 = program.getInstruction(offset + 1);

        // Match: STORE_GLOBAL X, LOAD_GLOBAL X (same variable)
        if (i1.opcode == OpCode::STORE_GLOBAL && i2.opcode == OpCode::LOAD_GLOBAL)
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
