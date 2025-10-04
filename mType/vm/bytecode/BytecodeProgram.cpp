#include "BytecodeProgram.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace vm::bytecode
{
    // === Instruction Implementation ===

    BytecodeProgram::Instruction::Instruction() : opcode(OpCode::NOP) {}

    BytecodeProgram::Instruction::Instruction(OpCode op) : opcode(op) {}

    BytecodeProgram::Instruction::Instruction(OpCode op, uint32_t operand1)
        : opcode(op), operands{operand1} {}

    BytecodeProgram::Instruction::Instruction(OpCode op, uint32_t operand1, uint32_t operand2)
        : opcode(op), operands{operand1, operand2} {}

    BytecodeProgram::Instruction::Instruction(OpCode op, std::vector<uint32_t> ops)
        : opcode(op), operands(std::move(ops)) {}

    // === ConstantPool Implementation ===

    size_t BytecodeProgram::ConstantPool::addInteger(int value) {
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
            return it->second;  // Deduplicate strings
        }
        size_t index = strings.size();
        strings.push_back(value);
        stringIndexMap[value] = index;
        return index;
    }

    int BytecodeProgram::ConstantPool::getInteger(size_t index) const {
        return integers.at(index);
    }

    double BytecodeProgram::ConstantPool::getFloat(size_t index) const {
        return floats.at(index);
    }

    const std::string& BytecodeProgram::ConstantPool::getString(size_t index) const {
        return strings.at(index);
    }

    // === BytecodeProgram Implementation ===

    BytecodeProgram::BytecodeProgram() : entryPoint(0) {}

    void BytecodeProgram::emit(OpCode opcode) {
        instructions.emplace_back(opcode);
    }

    void BytecodeProgram::emit(OpCode opcode, uint32_t operand) {
        instructions.emplace_back(opcode, operand);
    }

    void BytecodeProgram::emit(OpCode opcode, uint32_t operand1, uint32_t operand2) {
        instructions.emplace_back(opcode, operand1, operand2);
    }

    void BytecodeProgram::emit(OpCode opcode, const std::vector<uint32_t>& operands) {
        instructions.emplace_back(opcode, operands);
    }

    size_t BytecodeProgram::getCurrentOffset() const {
        return instructions.size();
    }

    void BytecodeProgram::patchJump(size_t instructionIndex, uint32_t targetOffset) {
        if (instructionIndex < instructions.size() && !instructions[instructionIndex].operands.empty()) {
            instructions[instructionIndex].operands[0] = targetOffset;
        }
    }

    const BytecodeProgram::Instruction& BytecodeProgram::getInstruction(size_t offset) const {
        return instructions.at(offset);
    }

    const std::vector<BytecodeProgram::Instruction>& BytecodeProgram::getInstructions() const {
        return instructions;
    }

    size_t BytecodeProgram::getInstructionCount() const {
        return instructions.size();
    }

    BytecodeProgram::ConstantPool& BytecodeProgram::getConstantPool() {
        return constantPool;
    }

    const BytecodeProgram::ConstantPool& BytecodeProgram::getConstantPool() const {
        return constantPool;
    }

    void BytecodeProgram::registerFunction(const std::string& name, const FunctionMetadata& metadata) {
        functions[name] = metadata;
    }

    const BytecodeProgram::FunctionMetadata* BytecodeProgram::getFunction(const std::string& name) const {
        auto it = functions.find(name);
        return it != functions.end() ? &it->second : nullptr;
    }

    const std::unordered_map<std::string, BytecodeProgram::FunctionMetadata>& BytecodeProgram::getFunctions() const {
        return functions;
    }

    void BytecodeProgram::addSourceLocation(size_t instructionOffset, uint32_t line, uint32_t column, const std::string& filename) {
        sourceLocations[instructionOffset] = {line, column, filename};
    }

    const BytecodeProgram::SourceLocation* BytecodeProgram::getSourceLocation(size_t instructionOffset) const {
        auto it = sourceLocations.find(instructionOffset);
        return it != sourceLocations.end() ? &it->second : nullptr;
    }

    void BytecodeProgram::setEntryPoint(size_t offset) {
        entryPoint = offset;
    }

    size_t BytecodeProgram::getEntryPoint() const {
        return entryPoint;
    }

    std::string BytecodeProgram::disassemble() const {
        std::ostringstream oss;
        oss << "=== Bytecode Disassembly ===\n";
        oss << "Entry Point: " << entryPoint << "\n\n";

        oss << "=== Constant Pool ===\n";
        oss << "Integers (" << constantPool.integers.size() << "):\n";
        for (size_t i = 0; i < constantPool.integers.size(); ++i) {
            oss << "  [" << i << "] " << constantPool.integers[i] << "\n";
        }
        oss << "Floats (" << constantPool.floats.size() << "):\n";
        for (size_t i = 0; i < constantPool.floats.size(); ++i) {
            oss << "  [" << i << "] " << constantPool.floats[i] << "\n";
        }
        oss << "Strings (" << constantPool.strings.size() << "):\n";
        for (size_t i = 0; i < constantPool.strings.size(); ++i) {
            oss << "  [" << i << "] \"" << constantPool.strings[i] << "\"\n";
        }

        oss << "\n=== Functions ===\n";
        for (const auto& [name, func] : functions) {
            oss << func.name << " (offset: " << func.startOffset
                << ", locals: " << func.localCount
                << ", params: " << func.parameterCount << ")\n";
        }

        oss << "\n=== Instructions ===\n";
        for (size_t i = 0; i < instructions.size(); ++i) {
            const auto& instr = instructions[i];
            oss << std::setw(6) << std::setfill('0') << i << ": "
                << std::setw(20) << std::setfill(' ') << std::left
                << getOpCodeName(instr.opcode);

            for (size_t j = 0; j < instr.operands.size(); ++j) {
                oss << " " << instr.operands[j];
            }

            // Add source location if available
            auto* loc = getSourceLocation(i);
            if (loc) {
                oss << "  ; " << loc->filename << ":" << loc->line;
            }
            oss << "\n";
        }

        return oss.str();
    }

    void BytecodeProgram::serialize(std::ostream& out) const {
        // Write magic number
        uint32_t magic = 0x4D545950;  // "MTYP"
        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

        // Write version
        uint32_t version = 1;
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));

        // Write entry point
        out.write(reinterpret_cast<const char*>(&entryPoint), sizeof(entryPoint));

        // Write constant pool
        writeConstantPool(out);

        // Write instructions
        writeInstructions(out);

        // Write functions
        writeFunctions(out);

        // Write source locations
        writeSourceLocations(out);
    }

    BytecodeProgram BytecodeProgram::deserialize(std::istream& in) {
        BytecodeProgram program;

        // Read and verify magic number
        uint32_t magic;
        in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x4D545950) {
            throw std::runtime_error("Invalid bytecode file: bad magic number");
        }

        // Read version
        uint32_t version;
        in.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (version != 1) {
            throw std::runtime_error("Unsupported bytecode version");
        }

        // Read entry point
        in.read(reinterpret_cast<char*>(&program.entryPoint), sizeof(program.entryPoint));

        // Read constant pool
        program.readConstantPool(in);

        // Read instructions
        program.readInstructions(in);

        // Read functions
        program.readFunctions(in);

        // Read source locations
        program.readSourceLocations(in);

        return program;
    }

    // === Private Serialization Helpers ===

    void BytecodeProgram::writeConstantPool(std::ostream& out) const {
        // Write integers
        size_t count = constantPool.integers.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        out.write(reinterpret_cast<const char*>(constantPool.integers.data()),
                 count * sizeof(int));

        // Write floats
        count = constantPool.floats.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        out.write(reinterpret_cast<const char*>(constantPool.floats.data()),
                 count * sizeof(double));

        // Write strings
        count = constantPool.strings.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& str : constantPool.strings) {
            size_t len = str.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(str.data(), len);
        }
    }

    void BytecodeProgram::readConstantPool(std::istream& in) {
        // Read integers
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        constantPool.integers.resize(count);
        in.read(reinterpret_cast<char*>(constantPool.integers.data()),
               count * sizeof(int));

        // Read floats
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        constantPool.floats.resize(count);
        in.read(reinterpret_cast<char*>(constantPool.floats.data()),
               count * sizeof(double));

        // Read strings
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        constantPool.strings.resize(count);
        for (size_t i = 0; i < count; ++i) {
            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string str(len, '\0');
            in.read(&str[0], len);
            constantPool.strings[i] = std::move(str);
            constantPool.stringIndexMap[constantPool.strings[i]] = i;
        }
    }

    void BytecodeProgram::writeInstructions(std::ostream& out) const {
        size_t count = instructions.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& instr : instructions) {
            out.write(reinterpret_cast<const char*>(&instr.opcode), sizeof(instr.opcode));
            size_t opCount = instr.operands.size();
            out.write(reinterpret_cast<const char*>(&opCount), sizeof(opCount));
            if (opCount > 0) {
                out.write(reinterpret_cast<const char*>(instr.operands.data()),
                         opCount * sizeof(uint32_t));
            }
        }
    }

    void BytecodeProgram::readInstructions(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        instructions.resize(count);
        for (size_t i = 0; i < count; ++i) {
            in.read(reinterpret_cast<char*>(&instructions[i].opcode),
                   sizeof(instructions[i].opcode));
            size_t opCount;
            in.read(reinterpret_cast<char*>(&opCount), sizeof(opCount));
            if (opCount > 0) {
                instructions[i].operands.resize(opCount);
                in.read(reinterpret_cast<char*>(instructions[i].operands.data()),
                       opCount * sizeof(uint32_t));
            }
        }
    }

    void BytecodeProgram::writeFunctions(std::ostream& out) const {
        size_t count = functions.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& [name, func] : functions) {
            // Write function name
            size_t len = name.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(name.data(), len);

            // Write function metadata
            out.write(reinterpret_cast<const char*>(&func.startOffset), sizeof(func.startOffset));
            out.write(reinterpret_cast<const char*>(&func.instructionCount), sizeof(func.instructionCount));
            out.write(reinterpret_cast<const char*>(&func.localCount), sizeof(func.localCount));
            out.write(reinterpret_cast<const char*>(&func.parameterCount), sizeof(func.parameterCount));
            out.write(reinterpret_cast<const char*>(&func.isStatic), sizeof(func.isStatic));
            out.write(reinterpret_cast<const char*>(&func.isNative), sizeof(func.isNative));

            // Write return type
            len = func.returnType.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(func.returnType.data(), len);

            // Write parameter names
            size_t paramCount = func.parameterNames.size();
            out.write(reinterpret_cast<const char*>(&paramCount), sizeof(paramCount));
            for (const auto& paramName : func.parameterNames) {
                len = paramName.size();
                out.write(reinterpret_cast<const char*>(&len), sizeof(len));
                out.write(paramName.data(), len);
            }
        }
    }

    void BytecodeProgram::readFunctions(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        for (size_t i = 0; i < count; ++i) {
            // Read function name
            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string name(len, '\0');
            in.read(&name[0], len);

            // Read function metadata
            FunctionMetadata func;
            func.name = name;
            in.read(reinterpret_cast<char*>(&func.startOffset), sizeof(func.startOffset));
            in.read(reinterpret_cast<char*>(&func.instructionCount), sizeof(func.instructionCount));
            in.read(reinterpret_cast<char*>(&func.localCount), sizeof(func.localCount));
            in.read(reinterpret_cast<char*>(&func.parameterCount), sizeof(func.parameterCount));
            in.read(reinterpret_cast<char*>(&func.isStatic), sizeof(func.isStatic));
            in.read(reinterpret_cast<char*>(&func.isNative), sizeof(func.isNative));

            // Read return type
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string returnType(len, '\0');
            in.read(&returnType[0], len);
            func.returnType = returnType;

            // Read parameter names
            size_t paramCount;
            in.read(reinterpret_cast<char*>(&paramCount), sizeof(paramCount));
            func.parameterNames.resize(paramCount);
            for (size_t j = 0; j < paramCount; ++j) {
                in.read(reinterpret_cast<char*>(&len), sizeof(len));
                std::string paramName(len, '\0');
                in.read(&paramName[0], len);
                func.parameterNames[j] = paramName;
            }

            functions[name] = func;
        }
    }

    void BytecodeProgram::writeSourceLocations(std::ostream& out) const {
        size_t count = sourceLocations.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& [offset, loc] : sourceLocations) {
            out.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
            out.write(reinterpret_cast<const char*>(&loc.line), sizeof(loc.line));
            out.write(reinterpret_cast<const char*>(&loc.column), sizeof(loc.column));

            size_t len = loc.filename.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(loc.filename.data(), len);
        }
    }

    void BytecodeProgram::readSourceLocations(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        for (size_t i = 0; i < count; ++i) {
            size_t offset;
            in.read(reinterpret_cast<char*>(&offset), sizeof(offset));

            SourceLocation loc;
            in.read(reinterpret_cast<char*>(&loc.line), sizeof(loc.line));
            in.read(reinterpret_cast<char*>(&loc.column), sizeof(loc.column));

            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string filename(len, '\0');
            in.read(&filename[0], len);
            loc.filename = filename;

            sourceLocations[offset] = loc;
        }
    }
}
