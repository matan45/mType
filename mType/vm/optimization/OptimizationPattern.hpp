#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../bytecode/BytecodeProgram.hpp"

namespace vm::optimization
{
    // Forward declarations
    namespace analysis
    {
        class ControlFlowAnalyzer;
    }

    /**
     * Abstract base class for all peephole optimization patterns
     * Follows Open/Closed Principle: open for extension, closed for modification
     * Each pattern is a self-contained transformation rule
     */
    class OptimizationPattern
    {
    public:
        /**
         * Replacement structure returned by apply()
         * Contains the new instructions and information about the transformation
         */
        struct Replacement
        {
            std::vector<bytecode::BytecodeProgram::Instruction> instructions;
            size_t originalLength;  // Number of original instructions replaced
            bool sourceLocationPreserved = true;  // Whether to preserve source location from first instruction

            Replacement() : originalLength(0) {}
            Replacement(size_t origLen) : originalLength(origLen) {}
        };

        virtual ~OptimizationPattern() = default;

        /**
         * Check if this pattern matches at the given offset
         * @param program The bytecode program to analyze
         * @param offset The instruction offset to check
         * @param cfg Control flow analyzer (may be used for safety checks)
         * @return true if pattern matches and can be safely applied
         */
        virtual bool matches(const bytecode::BytecodeProgram& program,
                           size_t offset,
                           const analysis::ControlFlowAnalyzer& cfg) const = 0;

        /**
         * Apply the optimization at the given offset
         * PRECONDITION: matches() returned true for this offset
         * @param program The bytecode program (const for reading constants/instructions)
         * @param offset The instruction offset where pattern was matched
         * @return Replacement containing new instructions and metadata
         */
        virtual Replacement apply(const bytecode::BytecodeProgram& program,
                                 size_t offset) const = 0;

        /**
         * Get the name of this optimization pattern (for debugging/statistics)
         */
        virtual std::string getName() const = 0;

        /**
         * Get the priority of this pattern (higher = runs earlier)
         * Default: 0 (normal priority)
         * Use higher values for patterns that enable other optimizations
         */
        virtual int getPriority() const { return 0; }

        /**
         * Get a description of this optimization pattern
         */
        virtual std::string getDescription() const { return getName(); }

    protected:
        /**
         * Helper: Check if an instruction is a constant push instruction
         */
        static bool isConstantPush(bytecode::OpCode opcode);

        /**
         * Helper: Check if an instruction is a jump instruction
         */
        static bool isJumpInstruction(bytecode::OpCode opcode);

        /**
         * Helper: Check if an instruction is a binary arithmetic operation
         */
        static bool isBinaryArithmetic(bytecode::OpCode opcode);

        /**
         * Helper: Check if an instruction is a unary operation
         */
        static bool isUnaryOperation(bytecode::OpCode opcode);

        /**
         * Helper: Check if an instruction is a comparison operation
         */
        static bool isComparisonOperation(bytecode::OpCode opcode);

        /**
         * Helper: Check if an instruction modifies control flow
         */
        static bool modifiesControlFlow(bytecode::OpCode opcode);

        /**
         * Helper: Check if an instruction has side effects
         */
        static bool hasSideEffects(bytecode::OpCode opcode);

        /**
         * Helper: Get the number of stack elements consumed by an instruction
         */
        static int getStackConsumption(bytecode::OpCode opcode);

        /**
         * Helper: Get the number of stack elements produced by an instruction
         */
        static int getStackProduction(bytecode::OpCode opcode);
    };

} // namespace vm::optimization
