#pragma once

#include "../../bytecode/OpCode.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace vm::runtime::optimization
{
    /**
     * Loop optimization metadata
     */
    struct LoopInfo {
        size_t startOffset;           // Offset of LOOP_START opcode
        size_t endOffset;             // Offset of LOOP_END opcode
        size_t loopConditionOffset;   // Where loop condition check starts
        size_t loopBackJumpOffset;    // Where JUMP_BACK occurs

        // Induction variable info (for simple counted loops)
        bool isCountedLoop = false;
        size_t inductionVarSlot = SIZE_MAX;
        int initialValue = 0;
        int stepValue = 1;
        int limitValue = 0;

        // Loop-invariant instructions (can be hoisted out of loop)
        std::vector<size_t> invariantInstructions;

        // Variables modified in the loop
        std::unordered_set<size_t> modifiedLocals;
        std::unordered_set<std::string> modifiedGlobals;
    };

    /**
     * @brief Loop optimizer for bytecode VM
     *
     * Performs optimizations:
     * 1. Counted loop detection (for i = 0; i < n; i++)
     * 2. Loop invariant code motion (LICM)
     * 3. Strength reduction (replace multiply with add in induction variables)
     * 4. Induction variable optimization
     *
     * Design Principles:
     * - Single Responsibility: Only loop optimizations
     * - Works on bytecode (post-compilation optimization)
     * - Non-destructive (can be disabled for debugging)
     */
    class LoopOptimizer {
    private:
        bytecode::BytecodeProgram& program;
        std::vector<LoopInfo> loops;

        /**
         * Find all loops in the program by scanning for LOOP_START/LOOP_END markers
         */
        void findLoops();

        /**
         * Analyze loop to detect if it's a simple counted loop
         * Pattern: for (int i = start; i < limit; i += step)
         */
        bool analyzeCountedLoop(LoopInfo& loop);

        /**
         * Find loop-invariant code (instructions that don't depend on loop variables)
         */
        void findInvariantCode(LoopInfo& loop);

        /**
         * Track which variables are modified in the loop body
         */
        void analyzeModifiedVariables(LoopInfo& loop);

        /**
         * Check if an instruction is invariant (doesn't depend on loop-modified variables)
         */
        bool isInstructionInvariant(size_t offset, const LoopInfo& loop) const;

        /**
         * Apply strength reduction: replace i*step with repeated addition
         */
        void applyStrengthReduction(LoopInfo& loop);

        /**
         * Hoist loop-invariant code outside the loop
         */
        void hoistInvariantCode(LoopInfo& loop);

    public:
        explicit LoopOptimizer(bytecode::BytecodeProgram& prog)
            : program(prog)
        {
        }

        /**
         * Run all loop optimizations on the bytecode program
         * Call this after compilation, before execution
         */
        void optimize();

        /**
         * Get loop information (for debugging/profiling)
         */
        const std::vector<LoopInfo>& getLoops() const { return loops; }

        /**
         * Check if optimizations were applied to any loops
         */
        bool hasOptimizations() const;
    };

} // namespace vm::runtime::optimization
