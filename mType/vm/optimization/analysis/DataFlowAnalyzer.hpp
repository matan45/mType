#pragma once
#include <vector>
#include <unordered_map>
#include "../../bytecode/BytecodeProgram.hpp"

namespace vm::optimization::analysis
{
    /**
     * Performs basic data flow analysis for optimization
     * Tracks stack effects and value definitions
     * Single Responsibility: Data flow analysis
     */
    class DataFlowAnalyzer
    {
    public:
        /**
         * Stack effect information for an instruction
         */
        struct StackEffect
        {
            int consumed;  // Number of stack elements consumed
            int produced;  // Number of stack elements produced
            int netEffect; // Net stack change (produced - consumed)

            StackEffect(int c = 0, int p = 0) : consumed(c), produced(p), netEffect(p - c) {}
        };

        DataFlowAnalyzer() = default;

        /**
         * Analyze a bytecode program for data flow
         * @param program The bytecode program to analyze
         */
        void analyze(const bytecode::BytecodeProgram& program);

        /**
         * Get the stack effect of an instruction at a given offset
         * @param offset The instruction offset
         * @return Stack effect information
         */
        StackEffect getStackEffect(size_t offset) const;

        /**
         * Calculate the stack effect of a specific opcode
         * @param opcode The opcode to analyze
         * @return Stack effect information
         */
        static StackEffect calculateStackEffect(bytecode::OpCode opcode);

        /**
         * Check if an instruction produces a constant value
         * @param offset The instruction offset
         * @return true if the instruction pushes a constant
         */
        bool producesConstant(size_t offset) const;

        /**
         * Clear all analyzed data
         */
        void clear();

    private:
        std::unordered_map<size_t, StackEffect> stackEffects;  // Stack effects by offset
        std::unordered_map<size_t, bool> constantProducers;    // Constant-producing instructions
    };

} // namespace vm::optimization::analysis
