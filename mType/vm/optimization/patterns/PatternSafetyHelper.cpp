#include "PatternSafetyHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include <sstream>

namespace vm::optimization::patterns
{
    using namespace bytecode;

    void PatternSafetyHelper::validateOperandExists(const BytecodeProgram::Instruction& instr,
                                                     size_t operandIndex,
                                                     size_t instrOffset)
    {
        if (operandIndex >= instr.operands.size())
        {
            std::ostringstream oss;
            oss << "Pattern safety error: Instruction operand access out of bounds. "
                << "Opcode: " << getOpCodeName(instr.opcode)
                << ", Instruction offset: " << instrOffset
                << ", Requested operand index: " << operandIndex
                << ", Available operands: " << instr.operands.size()
                << ". This indicates malformed bytecode or a bug in the optimizer.";
            throw std::runtime_error(oss.str());
        }
    }

    int PatternSafetyHelper::safeGetInteger(const BytecodeProgram::ConstantPool& pool,
                                            const BytecodeProgram::Instruction& instr,
                                            size_t operandIndex,
                                            size_t instrOffset)
    {
        // First, validate the operand exists
        validateOperandExists(instr, operandIndex, instrOffset);

        // Get the constant pool index
        uint32_t poolIndex = instr.operands[operandIndex];

        // Access constant pool (throws std::out_of_range if invalid)
        try
        {
            return pool.getInteger(poolIndex);
        }
        catch (const std::out_of_range&)
        {
            std::ostringstream oss;
            oss << "Pattern safety error: Constant pool integer access out of bounds. "
                << "Opcode: " << getOpCodeName(instr.opcode)
                << ", Instruction offset: " << instrOffset
                << ", Pool index: " << poolIndex
                << ". This indicates corrupted bytecode or invalid constant pool reference.";
            throw std::runtime_error(oss.str());
        }
    }

    double PatternSafetyHelper::safeGetFloat(const BytecodeProgram::ConstantPool& pool,
                                             const BytecodeProgram::Instruction& instr,
                                             size_t operandIndex,
                                             size_t instrOffset)
    {
        // First, validate the operand exists
        validateOperandExists(instr, operandIndex, instrOffset);

        // Get the constant pool index
        uint32_t poolIndex = instr.operands[operandIndex];

        // Access constant pool (throws std::out_of_range if invalid)
        try
        {
            return pool.getFloat(poolIndex);
        }
        catch (const std::out_of_range&)
        {
            std::ostringstream oss;
            oss << "Pattern safety error: Constant pool float access out of bounds. "
                << "Opcode: " << getOpCodeName(instr.opcode)
                << ", Instruction offset: " << instrOffset
                << ", Pool index: " << poolIndex
                << ". This indicates corrupted bytecode or invalid constant pool reference.";
            throw std::runtime_error(oss.str());
        }
    }

    const std::string& PatternSafetyHelper::safeGetString(const BytecodeProgram::ConstantPool& pool,
                                                          const BytecodeProgram::Instruction& instr,
                                                          size_t operandIndex,
                                                          size_t instrOffset)
    {
        // First, validate the operand exists
        validateOperandExists(instr, operandIndex, instrOffset);

        // Get the constant pool index
        uint32_t poolIndex = instr.operands[operandIndex];

        // Access constant pool (throws std::out_of_range if invalid)
        try
        {
            return pool.getString(poolIndex);
        }
        catch (const std::out_of_range&)
        {
            std::ostringstream oss;
            oss << "Pattern safety error: Constant pool string access out of bounds. "
                << "Opcode: " << getOpCodeName(instr.opcode)
                << ", Instruction offset: " << instrOffset
                << ", Pool index: " << poolIndex
                << ". This indicates corrupted bytecode or invalid constant pool reference.";
            throw std::runtime_error(oss.str());
        }
    }

    uint32_t PatternSafetyHelper::safeGetOperand(const BytecodeProgram::Instruction& instr,
                                                 size_t operandIndex,
                                                 size_t instrOffset)
    {
        validateOperandExists(instr, operandIndex, instrOffset);
        return instr.operands[operandIndex];
    }

} // namespace vm::optimization::patterns
