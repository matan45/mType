#pragma once
#include "../OptimizationPattern.hpp"
#include <cstddef>

namespace vm::optimization::patterns
{
    /**
     * Algebraic Simplification Pattern
     * Applies algebraic identities to simplify operations
     *
     * Examples:
     * - X + 0 -> X
     * - X * 1 -> X
     * - X * 0 -> 0
     * - X - 0 -> X
     * - 0 - X -> -X
     *
     * Priority: Medium (70)
     */
    class AlgebraicSimplificationPattern : public OptimizationPattern
    {
    public:
        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "AlgebraicSimplification"; }
        int getPriority() const override { return 70; }

    private:
        // Match: X + 0 or 0 + X
        bool matchesAddZero(const bytecode::BytecodeProgram& program,
                           size_t offset,
                           const analysis::ControlFlowAnalyzer& cfg) const;

        // Match: X * 1 or 1 * X
        bool matchesMulOne(const bytecode::BytecodeProgram& program,
                          size_t offset,
                          const analysis::ControlFlowAnalyzer& cfg) const;

        // Match: X * 0 or 0 * X
        bool matchesMulZero(const bytecode::BytecodeProgram& program,
                           size_t offset,
                           const analysis::ControlFlowAnalyzer& cfg) const;

        // Match: X - 0
        bool matchesSubZero(const bytecode::BytecodeProgram& program,
                           size_t offset,
                           const analysis::ControlFlowAnalyzer& cfg) const;
    };

} // namespace vm::optimization::patterns
