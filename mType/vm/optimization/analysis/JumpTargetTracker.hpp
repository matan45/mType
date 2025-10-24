#pragma once
#include <unordered_set>
#include <vector>
#include "../../bytecode/BytecodeProgram.hpp"

namespace vm::optimization::analysis
{
    /**
     * Tracks all jump targets in a bytecode program
     * Essential for identifying basic block boundaries and ensuring safe optimizations
     * Single Responsibility: Track and query jump targets
     */
    class JumpTargetTracker
    {
    public:
        JumpTargetTracker() = default;

        /**
         * Analyze a bytecode program and identify all jump targets
         * @param program The bytecode program to analyze
         */
        void analyze(const bytecode::BytecodeProgram& program);

        /**
         * Check if a given offset is a jump target
         * @param offset The instruction offset to check
         * @return true if this offset is targeted by any jump instruction
         */
        bool isJumpTarget(size_t offset) const;

        /**
         * Check if a given offset is an exception handler entry point
         * @param offset The instruction offset to check
         * @return true if this offset is a TRY_BEGIN, CATCH, or FINALLY target
         */
        bool isExceptionTarget(size_t offset) const;

        /**
         * Check if a range of instructions can be safely optimized
         * Returns false if any interior offsets (not start) are jump targets
         * @param start Start offset (inclusive)
         * @param end End offset (exclusive)
         * @return true if the range can be optimized safely
         */
        bool canOptimizeRange(size_t start, size_t end) const;

        /**
         * Get all jump targets in the program
         * @return Set of all jump target offsets
         */
        const std::unordered_set<size_t>& getAllTargets() const { return jumpTargets; }

        /**
         * Get all function entry points
         * @return Set of all function start offsets
         */
        const std::unordered_set<size_t>& getFunctionEntryPoints() const { return functionEntries; }

        /**
         * Clear all tracked data
         */
        void clear();

    private:
        std::unordered_set<size_t> jumpTargets;        // All jump targets
        std::unordered_set<size_t> exceptionTargets;   // Exception handler targets
        std::unordered_set<size_t> functionEntries;    // Function entry points
        std::unordered_set<size_t> loopHeaders;        // Loop header offsets (LOOP_START)

        /**
         * Helper: Scan all jump instructions and record their targets
         */
        void scanJumpInstructions(const bytecode::BytecodeProgram& program);

        /**
         * Helper: Scan all function metadata and record entry points
         */
        void scanFunctionEntries(const bytecode::BytecodeProgram& program);

        /**
         * Helper: Scan exception handling constructs
         */
        void scanExceptionHandlers(const bytecode::BytecodeProgram& program);

        /**
         * Helper: Scan loop markers
         */
        void scanLoopMarkers(const bytecode::BytecodeProgram& program);

        /**
         * Helper: Calculate jump target from a jump instruction
         */
        size_t calculateJumpTarget(size_t instructionOffset, const bytecode::BytecodeProgram::Instruction& instr) const;

        /**
         * Helper: Check if an opcode is a jump instruction
         */
        static bool isJumpOpCode(bytecode::OpCode opcode);
    };

} // namespace vm::optimization::analysis
