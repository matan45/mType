#pragma once
#include "../OptimizationPattern.hpp"

namespace vm::optimization::patterns
{
    /**
     * Null Sequence Pattern
     * Removes operations that cancel each other out
     *
     * Examples:
     * - NEG, NEG → (removed)
     * - NOT, NOT → (removed)
     *
     * Priority: Medium (45)
     */
    class NullSequencePattern : public OptimizationPattern
    {
    public:
        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "NullSequence"; }
        int getPriority() const override { return 45; }

    private:
        // Match: NEG, NEG → (removed)
        bool matchesDoubleNeg(const bytecode::BytecodeProgram& program,
                             size_t offset,
                             const analysis::ControlFlowAnalyzer& cfg) const;

        // Match: NOT, NOT → (removed)
        bool matchesDoubleNot(const bytecode::BytecodeProgram& program,
                             size_t offset,
                             const analysis::ControlFlowAnalyzer& cfg) const;
    };

} // namespace vm::optimization::patterns
