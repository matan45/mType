#pragma once
#include "../OptimizationPattern.hpp"

namespace vm::optimization::patterns
{
    /**
     * Load/Store Optimization Pattern
     * Optimizes redundant memory access patterns
     *
     * Examples:
     * - STORE_LOCAL X, LOAD_LOCAL X → STORE_LOCAL X, DUP
     * - STORE_GLOBAL X, LOAD_GLOBAL X → STORE_GLOBAL X, DUP
     *
     * Priority: Medium (55)
     */
    class LoadStoreOptimizationPattern : public OptimizationPattern
    {
    public:
        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "LoadStoreOptimization"; }
        int getPriority() const override { return 55; }

    private:
        // Match: STORE_LOCAL X, LOAD_LOCAL X → STORE_LOCAL X, DUP
        bool matchesStoreLoadLocal(const bytecode::BytecodeProgram& program,
                                   size_t offset,
                                   const analysis::ControlFlowAnalyzer& cfg) const;

        // Match: STORE_GLOBAL X, LOAD_GLOBAL X → STORE_GLOBAL X, DUP
        bool matchesStoreLoadGlobal(const bytecode::BytecodeProgram& program,
                                    size_t offset,
                                    const analysis::ControlFlowAnalyzer& cfg) const;
    };

} // namespace vm::optimization::patterns
