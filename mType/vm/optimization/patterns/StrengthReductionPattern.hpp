#pragma once
#include "../OptimizationPattern.hpp"

namespace vm::optimization::patterns
{
    /**
     * Strength Reduction Pattern
     * Replaces expensive operations with cheaper equivalents
     *
     * Examples:
     * - X / 1 → X
     * - X / -1 → -X
     * - Multiplication by power of 2 could use shifts (if supported)
     *
     * Priority: Medium (50)
     */
    class StrengthReductionPattern : public OptimizationPattern
    {
    public:
        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "StrengthReduction"; }
        int getPriority() const override { return 50; }

    private:
        // Match: X / 1 → X
        bool matchesDivOne(const bytecode::BytecodeProgram& program,
                          size_t offset,
                          const analysis::ControlFlowAnalyzer& cfg) const;

        // Match: X / -1 → -X
        bool matchesDivNegOne(const bytecode::BytecodeProgram& program,
                             size_t offset,
                             const analysis::ControlFlowAnalyzer& cfg) const;

        // Helper: Check if a value is a power of 2
        static bool isPowerOfTwo(int value);
    };

} // namespace vm::optimization::patterns
