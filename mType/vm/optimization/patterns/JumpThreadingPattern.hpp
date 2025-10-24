#pragma once
#include "../OptimizationPattern.hpp"

namespace vm::optimization::patterns
{
    /**
     * Jump Threading Pattern
     * Optimizes jump chains by eliminating intermediate jumps
     *
     * Examples:
     * - JUMP target1; ... at target1: JUMP target2 → JUMP target2
     * - JUMP_IF_FALSE target1; ... at target1: JUMP target2 → JUMP_IF_FALSE target2
     *
     * Priority: Medium (60) - requires CFG analysis
     */
    class JumpThreadingPattern : public OptimizationPattern
    {
    public:
        bool matches(const bytecode::BytecodeProgram& program,
                    size_t offset,
                    const analysis::ControlFlowAnalyzer& cfg) const override;

        Replacement apply(const bytecode::BytecodeProgram& program,
                         size_t offset) const override;

        std::string getName() const override { return "JumpThreading"; }
        int getPriority() const override { return 60; }

    private:
        /**
         * Helper: Get the final target of a jump chain
         * Follows jumps until reaching a non-jump instruction
         * Returns the offset of the final target
         */
        size_t getFinalJumpTarget(const bytecode::BytecodeProgram& program,
                                  size_t startOffset,
                                  int maxDepth = 10) const;

        /**
         * Helper: Check if an instruction at offset is an unconditional jump
         */
        bool isUnconditionalJump(const bytecode::BytecodeProgram& program,
                                size_t offset) const;

        /**
         * Helper: Check if jump threading would be safe (no infinite loops)
         */
        bool isSafeToThread(const bytecode::BytecodeProgram& program,
                           size_t jumpOffset,
                           size_t newTarget) const;
    };

} // namespace vm::optimization::patterns
