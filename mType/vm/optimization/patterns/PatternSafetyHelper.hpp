#pragma once
#include "../../bytecode/BytecodeProgram.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace vm::optimization::patterns
{
    /**F
     * Safety helper for pattern implementations
     * Provides bounds-checked access to instruction operands and constant pool
     * Single Responsibility: Validate bytecode access in optimization patterns
     */
    class PatternSafetyHelper
    {
    public:
        /**
         * Safely get an integer from constant pool
         * @param pool The constant pool
         * @param instr The instruction containing the operand
         * @param operandIndex Which operand contains the pool index (default: 0)
         * @param instrOffset Instruction offset for error messages (default: 0)
         * @return The integer value
         * @throws std::runtime_error if operand doesn't exist or pool index is invalid
         */
        static int64_t safeGetInteger(const bytecode::BytecodeProgram::ConstantPool& pool,
                                  const bytecode::BytecodeProgram::Instruction& instr,
                                  size_t operandIndex = 0,
                                  size_t instrOffset = 0);

        /**
         * Safely get a float from constant pool
         * @param pool The constant pool
         * @param instr The instruction containing the operand
         * @param operandIndex Which operand contains the pool index (default: 0)
         * @param instrOffset Instruction offset for error messages (default: 0)
         * @return The float value
         * @throws std::runtime_error if operand doesn't exist or pool index is invalid
         */
        static double safeGetFloat(const bytecode::BytecodeProgram::ConstantPool& pool,
                                   const bytecode::BytecodeProgram::Instruction& instr,
                                   size_t operandIndex = 0,
                                   size_t instrOffset = 0);

        /**
         * Safely get a string from constant pool
         * @param pool The constant pool
         * @param instr The instruction containing the operand
         * @param operandIndex Which operand contains the pool index (default: 0)
         * @param instrOffset Instruction offset for error messages (default: 0)
         * @return The string value
         * @throws std::runtime_error if operand doesn't exist or pool index is invalid
         */
        static const std::string& safeGetString(const bytecode::BytecodeProgram::ConstantPool& pool,
                                                const bytecode::BytecodeProgram::Instruction& instr,
                                                size_t operandIndex = 0,
                                                size_t instrOffset = 0);

        /**
         * Safely get an operand value
         * @param instr The instruction
         * @param operandIndex Which operand to get (default: 0)
         * @param instrOffset Instruction offset for error messages (default: 0)
         * @return The operand value
         * @throws std::runtime_error if operand doesn't exist
         */
        static uint64_t safeGetOperand(const bytecode::BytecodeProgram::Instruction& instr,
                                       size_t operandIndex = 0,
                                       size_t instrOffset = 0);

    private:
        /**
         * Check if instruction has required operand
         * @throws std::runtime_error with detailed message if not
         */
        static void validateOperandExists(const bytecode::BytecodeProgram::Instruction& instr,
                                          size_t operandIndex,
                                          size_t instrOffset);
    };

} // namespace vm::optimization::patterns
