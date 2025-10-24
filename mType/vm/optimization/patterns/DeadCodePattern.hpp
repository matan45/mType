#pragma once
#include "../OptimizationPattern.hpp"

namespace vm::optimization::patterns
{
    /**
     * Dead Code Elimination Pattern
     * Removes unreachable code and useless operations
     *
     * Examples:
     * - PUSH_INT 5, POP -> (removed)
     * - Unreachable code after RETURN -> (removed)
     * - NOP -> (removed)
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

        // Match: Unreachable instruction
        bool matchesUnreachable(const bytecode::BytecodeProgram& program,
                               size_t offset,
                               const analysis::ControlFlowAnalyzer& cfg) const;
    };

} // namespace vm::optimization::patterns
