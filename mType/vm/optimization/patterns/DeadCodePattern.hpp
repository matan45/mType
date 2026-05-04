#pragma once
#include "../OptimizationPattern.hpp"
#include <cstddef>

namespace vm::optimization::patterns
{
    /**
     * Dead Code Elimination Pattern
     * Removes useless operations and explicitly unused values
     *
     * Currently handles:
     * - NOP instructions
     * - PUSH immediately followed by POP (unused values)
     *
     * Note: Unreachable code detection (after unconditional jumps/returns) is not
     * implemented as it requires tracking CALL targets and function boundaries
     * to avoid incorrectly removing function bodies that appear unreachable but
     * are called via CALL instructions.
     *
     * Priority: High (90)
     */
    class DeadCodePattern : public OptimizationPattern
    {
    public:
        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "DeadCode"; }
        int getPriority() const override { return 90; }

    private:
        // Match: Push followed by Pop (useless operation)
        bool matchesPushPop(const bytecode::BytecodeProgram& program,
                           size_t offset,
                           const analysis::ControlFlowAnalyzer& cfg) const;

        // Match: NOP instruction
        bool matchesNOP(const bytecode::BytecodeProgram& program,
                       size_t offset) const;
    };

} // namespace vm::optimization::patterns
