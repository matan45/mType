#pragma once
#include "../OptimizationPattern.hpp"

namespace vm::optimization::patterns
{
    /**
     * Stack Optimization Pattern
     * Optimizes stack manipulation sequences
     *
     * Examples:
     * - SWAP, SWAP → (removed)
     * - DUP, POP → (removed)
     * - LOAD X, LOAD X → LOAD X, DUP
     *
     * Priority: Low (40)
     */
    class StackOptimizationPattern : public OptimizationPattern
    {
    public:
        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "StackOptimization"; }
        int getPriority() const override { return 40; }

    private:
        // Match: SWAP, SWAP → (removed)
        bool matchesDoubleSwap(const bytecode::BytecodeProgram& program,
                              size_t offset,
                              const analysis::ControlFlowAnalyzer& cfg) const;

        // Match: DUP, POP → (removed)
        bool matchesDupPop(const bytecode::BytecodeProgram& program,
                          size_t offset,
                          const analysis::ControlFlowAnalyzer& cfg) const;

        // Match: LOAD X, LOAD X → LOAD X, DUP
        bool matchesDoubleLoad(const bytecode::BytecodeProgram& program,
                              size_t offset,
                              const analysis::ControlFlowAnalyzer& cfg) const;
    };

} // namespace vm::optimization::patterns
