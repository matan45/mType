#include "BytecodeProgram.hpp"
#include <cstddef>
#include <stdexcept>

namespace vm::bytecode
{
    // === Instruction ===

    BytecodeProgram::Instruction::Instruction() : opcode(OpCode::NOP) {}

    BytecodeProgram::Instruction::Instruction(OpCode op) : opcode(op) {}

    BytecodeProgram::Instruction::Instruction(OpCode op, uint64_t operand1)
        : opcode(op), operandCount(1)
    {
        inlineOperands[0] = operand1;
    }

    BytecodeProgram::Instruction::Instruction(OpCode op, uint64_t operand1, uint64_t operand2)
        : opcode(op), operandCount(2)
    {
        inlineOperands[0] = operand1;
        inlineOperands[1] = operand2;
    }

    BytecodeProgram::Instruction::Instruction(OpCode op, std::vector<uint64_t> ops)
        : opcode(op)
    {
        loadOperands(ops.data(), ops.size());
    }

    BytecodeProgram::Instruction::Instruction(const Instruction& other)
        : opcode(other.opcode), flags(other.flags), operandCount(other.operandCount)
    {
        inlineOperands[0] = other.inlineOperands[0];
        inlineOperands[1] = other.inlineOperands[1];
        inlineOperands[2] = other.inlineOperands[2];
        if (other.operandCount > 3) {
            size_t n = static_cast<size_t>(other.operandCount) - 3;
            overflow = std::make_unique<uint64_t[]>(n);
            for (size_t i = 0; i < n; ++i) overflow[i] = other.overflow[i];
        }
    }

    BytecodeProgram::Instruction& BytecodeProgram::Instruction::operator=(const Instruction& other)
    {
        if (this == &other) return *this;
        opcode = other.opcode;
        flags = other.flags;
        operandCount = other.operandCount;
        inlineOperands[0] = other.inlineOperands[0];
        inlineOperands[1] = other.inlineOperands[1];
        inlineOperands[2] = other.inlineOperands[2];
        if (other.operandCount > 3) {
            size_t n = static_cast<size_t>(other.operandCount) - 3;
            overflow = std::make_unique<uint64_t[]>(n);
            for (size_t i = 0; i < n; ++i) overflow[i] = other.overflow[i];
        } else {
            overflow.reset();
        }
        return *this;
    }

    void BytecodeProgram::Instruction::loadOperands(const uint64_t* src, size_t count)
    {
        operandCount = static_cast<uint8_t>(count);
        size_t inline_n = count < 3 ? count : 3;
        for (size_t i = 0; i < inline_n; ++i) inlineOperands[i] = src[i];
        // Zero unused inline slots so disassembly / debug dumps don't show stale data.
        for (size_t i = inline_n; i < 3; ++i) inlineOperands[i] = 0;
        if (count > 3) {
            size_t overflow_n = count - 3;
            overflow = std::make_unique<uint64_t[]>(overflow_n);
            for (size_t i = 0; i < overflow_n; ++i) overflow[i] = src[i + 3];
        } else {
            overflow.reset();
        }
    }

    // === ConstantPool ===

    size_t BytecodeProgram::ConstantPool::addInteger(int64_t value) {
        integers.push_back(value);
        return integers.size() - 1;
    }

    size_t BytecodeProgram::ConstantPool::addFloat(double value) {
        floats.push_back(value);
        return floats.size() - 1;
    }

    size_t BytecodeProgram::ConstantPool::addString(const std::string& value) {
        auto it = stringIndexMap.find(value);
        if (it != stringIndexMap.end()) {
            return it->second;
        }
        size_t index = strings.size();
        strings.push_back(value);
        stringIndexMap[value] = index;
        return index;
    }

    int64_t BytecodeProgram::ConstantPool::getInteger(size_t index) const {
        return integers.at(index);
    }

    double BytecodeProgram::ConstantPool::getFloat(size_t index) const {
        return floats.at(index);
    }

    const std::string& BytecodeProgram::ConstantPool::getString(size_t index) const {
        return strings.at(index);
    }

    // === BytecodeProgram ===

    BytecodeProgram::BytecodeProgram() : entryPoint(0) {}

    namespace
    {
        // MYT-318: per-opcode minimum operand count. Verified once at program
        // load by validateInstructionOperands(); executors covered here may
        // omit the runtime `if (numOperands() == 0) throw...` defensive check.
        //
        // Conservative — any opcode not listed defaults to 0 (no requirement,
        // preserving the existing defensive-check semantics for unaudited
        // opcodes). Runtime-only opcodes (CALL_METHOD_CACHED, LOAD_LOCAL_INT,
        // etc.) inherit their generic form's operand contract via in-place
        // rewrite (operands aren't mutated), so the same minimum applies.
        constexpr uint8_t opcodeMinOperands(OpCode op) noexcept
        {
            switch (op)
            {
            case OpCode::PUSH_INT:
            case OpCode::PUSH_FLOAT:
            case OpCode::PUSH_STRING:
            case OpCode::PUSH_BOOL:
                return 1;

            case OpCode::SWITCH_STRING:
                return 2;

            case OpCode::LOAD_LOCAL:
            case OpCode::LOAD_LOCAL_INT:
            case OpCode::LOAD_LOCAL_FLOAT:
            case OpCode::LOAD_LOCAL_BOOL:
            case OpCode::LOAD_LOCAL_BOXED_INST:
            case OpCode::STORE_LOCAL_INT:
            case OpCode::STORE_LOCAL_FLOAT:
            case OpCode::STORE_LOCAL_BOOL:
            case OpCode::STORE_LOCAL_BOXED_INST:
            case OpCode::LOAD_GLOBAL:
            case OpCode::STORE_GLOBAL:
            case OpCode::LOAD_VAR:
            case OpCode::LOAD_VAR_CACHED:
            case OpCode::DECLARE_VAR:
                return 1;

            // STORE_LOCAL: 1 operand minimum (slot), optional 2nd (varName).
            case OpCode::STORE_LOCAL:
            case OpCode::STORE_VAR:
            case OpCode::STORE_VAR_CACHED:
                return 1;

            case OpCode::GET_FIELD:
            case OpCode::SET_FIELD:
            case OpCode::GET_FIELD_FAST:
            case OpCode::SET_FIELD_FAST:
            case OpCode::INLINE_SET_FIELD:
            case OpCode::INLINE_GET_FIELD:
            case OpCode::GET_FIELD_CACHED:
            case OpCode::SET_FIELD_CACHED:
                return 1;

            case OpCode::JUMP:
            case OpCode::JUMP_IF_FALSE:
            case OpCode::JUMP_IF_TRUE:
            case OpCode::JUMP_IF_NULL:
            case OpCode::JUMP_BACK:
            case OpCode::JUMP_IF_FALSE_OR_POP:
            case OpCode::JUMP_IF_TRUE_OR_POP:
                return 1;

            // MYT-202 fused superinstructions.
            case OpCode::LOAD_STORE_LOCAL:
            case OpCode::LOAD_GET_FIELD:
            case OpCode::LOAD_LOAD_ADD_INT:
            case OpCode::LOAD_LOAD_SUB_INT:
            case OpCode::LOAD_LOAD_MUL_INT:
                return 2;
            case OpCode::ADD_INT_STORE_LOCAL:
                return 1;

            // Fused local-array ops (compiler-emitted).
            case OpCode::ARRAY_GET_INT_LOCAL:
            case OpCode::ARRAY_SET_INT_LOCAL:
            case OpCode::ARRAY_LENGTH_LOCAL:
                return 1;

            case OpCode::CALL:
            case OpCode::CALL_FAST:
            case OpCode::CALL_METHOD:
            case OpCode::CALL_METHOD_CACHED:
            case OpCode::CALL_METHOD_POLY_CACHED:
            case OpCode::CALL_STATIC:
            case OpCode::INVOKE:
            case OpCode::SUPER_INVOKE:
            case OpCode::TAIL_CALL:
                return 2;

            case OpCode::INSTANCEOF:
            case OpCode::INSTANCEOF_TYPEPARAM:
            case OpCode::CAST:
            case OpCode::CAST_TYPEPARAM:
            case OpCode::CHECK_TYPE:
                return 1;

            // Default 0 preserves existing defensive checks for unaudited opcodes.
            default:
                return 0;
            }
        }
    }

    void BytecodeProgram::validateInstructionOperands() const
    {
        for (size_t ip = 0; ip < instructions.size(); ++ip)
        {
            const auto& instr = instructions[ip];
            uint8_t required = opcodeMinOperands(instr.opcode);
            if (instr.numOperands() < required)
            {
                throw std::runtime_error(
                    std::string("Bytecode validation: opcode ") +
                    getOpCodeName(instr.opcode) +
                    " at instruction " + std::to_string(ip) +
                    " requires at least " + std::to_string(static_cast<int>(required)) +
                    " operand(s) but has " + std::to_string(instr.numOperands()));
            }

            if (instr.opcode == OpCode::SWITCH_STRING)
            {
                const size_t caseCount = static_cast<size_t>(instr.inlineOperands[1]);
                const size_t expected = 2 + caseCount * 2;
                if (instr.numOperands() != expected)
                {
                    throw std::runtime_error(
                        "Bytecode validation: SWITCH_STRING at instruction " +
                        std::to_string(ip) + " expects " + std::to_string(expected) +
                        " operand(s) from caseCount but has " +
                        std::to_string(instr.numOperands()));
                }
            }
        }
    }

    const uint64_t* BytecodeProgram::materializeStableOperandSlice(
        const Instruction& instr, size_t start, size_t count) const
    {
        // Operands are split inline/overflow at index 3, so a slice may straddle
        // the boundary. Always copy into a stable heap-owned buffer for the JIT
        // immediate. std::list node addresses (and the inner vector's data()
        // pointer, since we don't grow the vector after emplace_back) remain
        // stable across subsequent slice materializations.
        auto& buf = jitStableSlotPool.emplace_back(count);
        for (size_t i = 0; i < count; ++i) {
            buf[i] = instr.operandAt(start + i);
        }
        return buf.data();
    }

    void BytecodeProgram::emit(OpCode opcode) {
        instructions.emplace_back(opcode);
        invalidateFusionUnsafeTargets();
    }

    void BytecodeProgram::emit(OpCode opcode, uint64_t operand) {
        instructions.emplace_back(opcode, operand);
        invalidateFusionUnsafeTargets();
    }

    void BytecodeProgram::emit(OpCode opcode, uint64_t operand1, uint64_t operand2) {
        instructions.emplace_back(opcode, operand1, operand2);
        invalidateFusionUnsafeTargets();
    }

    void BytecodeProgram::emit(OpCode opcode, const std::vector<uint64_t>& operands) {
        instructions.emplace_back(opcode, operands);
        invalidateFusionUnsafeTargets();
    }

    size_t BytecodeProgram::getCurrentOffset() const {
        return instructions.size();
    }

    void BytecodeProgram::patchJump(size_t instructionIndex, uint64_t targetOffset) {
        if (instructionIndex < instructions.size() && instructions[instructionIndex].hasOperands()) {
            instructions[instructionIndex].setOperand0(targetOffset);
        }
    }

    void BytecodeProgram::setLastInstructionFlags(uint8_t flags) {
        if (!instructions.empty()) {
            instructions.back().flags = flags;
        }
    }

    const BytecodeProgram::Instruction& BytecodeProgram::getInstruction(size_t offset) const {
        return instructions.at(offset);
    }

    BytecodeProgram::Instruction& BytecodeProgram::getMutableInstruction(size_t offset) {
        return instructions.at(offset);
    }

    const std::vector<BytecodeProgram::Instruction>& BytecodeProgram::getInstructions() const {
        return instructions;
    }

    size_t BytecodeProgram::getInstructionCount() const {
        return instructions.size();
    }
}
