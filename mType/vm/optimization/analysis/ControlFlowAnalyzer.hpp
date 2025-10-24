#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "JumpTargetTracker.hpp"
#include "../../bytecode/BytecodeProgram.hpp"

namespace vm::optimization::analysis
{
    /**
     * Analyzes control flow and constructs a Control Flow Graph (CFG)
     * Identifies basic blocks, their predecessors and successors
     * Detects unreachable code
     * Single Responsibility: Control flow graph construction and analysis
     */
    class ControlFlowAnalyzer
    {
    public:
        /**
         * Represents a basic block in the control flow graph
         * A basic block is a straight-line sequence of instructions with:
         * - One entry point (only the first instruction can be jumped to)
         * - One exit point (only the last instruction can jump/branch)
         */
        struct BasicBlock
        {
            size_t startOffset;      // First instruction offset
            size_t endOffset;        // Last instruction offset (inclusive)
            std::vector<size_t> successors;    // Indices of successor basic blocks
            std::vector<size_t> predecessors;  // Indices of predecessor basic blocks
            bool isReachable = true; // Whether this block is reachable from entry

            BasicBlock(size_t start, size_t end)
                : startOffset(start), endOffset(end) {}
        };

        ControlFlowAnalyzer() = default;

        /**
         * Analyze a bytecode program and construct the CFG
         * @param program The bytecode program to analyze
         */
        void analyze(const bytecode::BytecodeProgram& program);

        /**
         * Check if a given offset is a jump target
         * @param offset The instruction offset to check
         * @return true if this offset is targeted by any jump
         */
        bool isJumpTarget(size_t offset) const;

        /**
         * Check if a given offset is the start of a basic block
         * @param offset The instruction offset to check
         * @return true if this offset starts a basic block
         */
        bool isBasicBlockStart(size_t offset) const;

        /**
         * Check if a given offset is the end of a basic block
         * @param offset The instruction offset to check
         * @return true if this offset ends a basic block
         */
        bool isBasicBlockEnd(size_t offset) const;

        /**
         * Get the basic block containing a given offset
         * @param offset The instruction offset
         * @return Pointer to the basic block, or nullptr if not found
         */
        const BasicBlock* getBasicBlock(size_t offset) const;

        /**
         * Get all basic blocks
         * @return Vector of all basic blocks
         */
        const std::vector<BasicBlock>& getAllBlocks() const { return blocks; }

        /**
         * Check if a range can be safely optimized
         * Returns false if optimization would cross basic block boundaries
         * @param start Start offset (inclusive)
         * @param end End offset (exclusive)
         * @return true if the range can be optimized
         */
        bool canOptimizeRange(size_t start, size_t end) const;

        /**
         * Check if an instruction offset is unreachable
         * @param offset The instruction offset
         * @return true if the instruction is unreachable
         */
        bool isUnreachable(size_t offset) const;

        /**
         * Get the jump target tracker
         * @return Reference to the jump target tracker
         */
        const JumpTargetTracker& getJumpTracker() const { return jumpTracker; }

        /**
         * Clear all analyzed data
         */
        void clear();

    private:
        std::vector<BasicBlock> blocks;                       // All basic blocks
        std::unordered_map<size_t, size_t> offsetToBlockIndex; // Maps offset to block index
        std::unordered_set<size_t> blockStarts;               // All basic block start offsets
        std::unordered_set<size_t> blockEnds;                 // All basic block end offsets
        std::unordered_set<size_t> unreachableOffsets;        // Unreachable instruction offsets
        JumpTargetTracker jumpTracker;                        // Jump target tracker

        /**
         * Step 1: Identify all basic block boundaries
         */
        void identifyBasicBlocks(const bytecode::BytecodeProgram& program);

        /**
         * Step 2: Build the control flow graph edges (predecessors/successors)
         */
        void buildControlFlowGraph(const bytecode::BytecodeProgram& program);

        /**
         * Step 3: Detect unreachable code via reachability analysis
         */
        void detectUnreachableCode(const bytecode::BytecodeProgram& program);

        /**
         * Helper: Add an edge from one block to another
         */
        void addEdge(size_t fromBlockIdx, size_t toBlockIdx);

        /**
         * Helper: Find block index containing a given offset
         */
        size_t findBlockIndex(size_t offset) const;

        /**
         * Helper: Check if an instruction terminates a basic block
         */
        static bool isBlockTerminator(bytecode::OpCode opcode);

        /**
         * Helper: Get the fall-through target of an instruction
         */
        size_t getFallThroughTarget(size_t offset, const bytecode::BytecodeProgram& program) const;

        /**
         * Helper: Get all branch targets of an instruction
         */
        std::vector<size_t> getBranchTargets(size_t offset,
                                             const bytecode::BytecodeProgram::Instruction& instr) const;
    };

} // namespace vm::optimization::analysis
