#include "BytecodeProgram.hpp"
#include "BytecodeIOHelper.hpp"
#include "BytecodeProgramReadValidators.hpp"
#include "../../constants/SecurityConstants.hpp"
#include <stdexcept>

namespace vm::bytecode
{
    namespace
    {
        // .mtc format version. MUST match BYTECODE_FORMAT_VERSION in
        // BytecodeProgram_SerializeWrite.cpp. Reader rejects any other value
        // with a "recompile" diagnostic so a stale .mtc cannot silently
        // misexecute against a newer interpreter.
        constexpr uint32_t BYTECODE_FORMAT_VERSION = 13;
    }

    BytecodeProgram BytecodeProgram::deserialize(std::istream& in) {
        BytecodeProgram program;

        uint32_t magic;
        in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x4D545950) {
            throw std::runtime_error("Invalid bytecode file: bad magic number");
        }

        uint32_t version;
        in.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (version != BYTECODE_FORMAT_VERSION) {
            throw std::runtime_error(
                "Bytecode file format version " + std::to_string(version) +
                " does not match the interpreter (expected " +
                std::to_string(BYTECODE_FORMAT_VERSION) + "). " +
                (version < BYTECODE_FORMAT_VERSION
                    ? "The .mtc was produced by an older compiler — recompile from source."
                    : "The .mtc was produced by a newer compiler — upgrade the interpreter."));
        }

        in.read(reinterpret_cast<char*>(&program.entryPoint), sizeof(program.entryPoint));

        program.readConstantPool(in);
        program.readInstructions(in);
        program.readFunctions(in);
        program.readSourceLocations(in);
        program.readClasses(in);
        program.readInterfaces(in);
        program.readGlobalExceptionTable(in);
        // MYT-108: annotation declarations (.mtc v5+).
        program.readAnnotationDeclarations(in);
        // MYT-290: static-initializer function names.
        program.staticInitializerFunctions = BytecodeIOHelper::readStringVector(in);
        // Top-level local names enable debugger variable inspection.
        program.topLevelLocalNames = BytecodeIOHelper::readStringVector(in);
        program.topLevelLocalCount = program.topLevelLocalNames.size();

        size_t len;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string sourcePath(len, '\0');
        in.read(&sourcePath[0], len);
        program.sourceFilePath = sourcePath;

        // MYT-318: validate the deserialized instruction stream so executors
        // can drop defensive operand-count checks on the hot dispatch path.
        program.validateInstructionOperands();

        return program;
    }

    void BytecodeProgram::readConstantPool(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        readv::validateCount(count, constants::security::MAX_CONSTANT_POOL_ENTRIES, "constant pool integers");
        constantPool.integers.resize(count);
        in.read(reinterpret_cast<char*>(constantPool.integers.data()),
               count * sizeof(int64_t));

        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        readv::validateCount(count, constants::security::MAX_CONSTANT_POOL_ENTRIES, "constant pool floats");
        constantPool.floats.resize(count);
        in.read(reinterpret_cast<char*>(constantPool.floats.data()),
               count * sizeof(double));

        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        readv::validateCount(count, constants::security::MAX_CONSTANT_POOL_ENTRIES, "constant pool strings");
        constantPool.strings.resize(count);
        for (size_t i = 0; i < count; ++i) {
            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            readv::validateStringLen(len, "constant pool string");
            std::string str(len, '\0');
            in.read(&str[0], len);
            constantPool.strings[i] = std::move(str);
            constantPool.stringIndexMap[constantPool.strings[i]] = i;
        }
    }

    void BytecodeProgram::readInstructions(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        readv::validateCount(count, constants::security::MAX_INSTRUCTION_COUNT, "instructions");
        instructions.resize(count);
        for (size_t i = 0; i < count; ++i) {
            in.read(reinterpret_cast<char*>(&instructions[i].opcode),
                   sizeof(instructions[i].opcode));
            if (!isValidOpCode(instructions[i].opcode)) {
                throw std::runtime_error(
                    "Bytecode deserialization rejected: invalid opcode " +
                    std::to_string(static_cast<unsigned>(instructions[i].opcode)) +
                    " at instruction " + std::to_string(i));
            }
            // MYT-173 / MYT-194: runtime-only specialized opcodes must never
            // appear in a serialized bytecode stream. They're the product of
            // interpreter-side opcode rewriting, have no compile-time emitter,
            // and their auxiliary state (cachedMethodShape / cachedFieldShape
            // et al.) is not serialized — so accepting them here would leave
            // the VM in an inconsistent state.
            //
            // MYT-202: compile-time superinstruction fusions (LOAD_LOAD_ADD_INT
            // et al.) are intentionally ABSENT from this list — they carry
            // their operands inside Instruction::operands and round-trip cleanly.
            if (instructions[i].opcode == OpCode::CALL_METHOD_CACHED ||
                instructions[i].opcode == OpCode::CALL_METHOD_POLY_CACHED ||
                instructions[i].opcode == OpCode::GET_FIELD_CACHED ||
                instructions[i].opcode == OpCode::SET_FIELD_CACHED ||
                instructions[i].opcode == OpCode::LOAD_VAR_CACHED ||
                instructions[i].opcode == OpCode::STORE_VAR_CACHED ||
                instructions[i].opcode == OpCode::ADD_INT_CONST ||
                instructions[i].opcode == OpCode::LOAD_LOCAL_CALL_CACHED ||
                instructions[i].opcode == OpCode::LOAD_LOCAL_CALL_POLY_CACHED ||
                instructions[i].opcode == OpCode::LOAD_LOCAL_GET_FIELD_CACHED ||
                instructions[i].opcode == OpCode::LOAD_LOCAL_INT ||
                instructions[i].opcode == OpCode::LOAD_LOCAL_FLOAT ||
                instructions[i].opcode == OpCode::LOAD_LOCAL_BOOL ||
                instructions[i].opcode == OpCode::LOAD_LOCAL_BOXED_INST ||
                instructions[i].opcode == OpCode::STORE_LOCAL_INT ||
                instructions[i].opcode == OpCode::STORE_LOCAL_FLOAT ||
                instructions[i].opcode == OpCode::STORE_LOCAL_BOOL ||
                instructions[i].opcode == OpCode::STORE_LOCAL_BOXED_INST ||
                // Runtime-only bitwise ops, emitted by trySpecializeBitwise.
                instructions[i].opcode == OpCode::BITWISE_AND_INT ||
                instructions[i].opcode == OpCode::BITWISE_OR_INT ||
                instructions[i].opcode == OpCode::BITWISE_XOR_INT ||
                instructions[i].opcode == OpCode::LEFT_SHIFT_INT ||
                instructions[i].opcode == OpCode::RIGHT_SHIFT_INT ||
                instructions[i].opcode == OpCode::BITWISE_NOT_INT) {
                throw std::runtime_error(
                    std::string("Bytecode deserialization rejected: runtime-only opcode ") +
                    getOpCodeName(instructions[i].opcode) +
                    " found at instruction " + std::to_string(i) +
                    " (this opcode is never emitted by the compiler and indicates "
                    "a malformed or tampered .mtc file)");
            }
            in.read(reinterpret_cast<char*>(&instructions[i].flags),
                   sizeof(instructions[i].flags));
            size_t opCount;
            in.read(reinterpret_cast<char*>(&opCount), sizeof(opCount));
            readv::validateCount(opCount, constants::security::MAX_OPERANDS_PER_INSTR, "instruction operands");
            if (opCount > 0) {
                std::vector<uint64_t> buf(opCount);
                in.read(reinterpret_cast<char*>(buf.data()),
                       opCount * sizeof(uint64_t));
                instructions[i].loadOperands(buf.data(), opCount);
            }
        }
    }

    void BytecodeProgram::readSourceLocations(std::istream& in) {
        size_t poolSize;
        in.read(reinterpret_cast<char*>(&poolSize), sizeof(poolSize));
        std::vector<std::string> filenamePool(poolSize);
        for (size_t i = 0; i < poolSize; ++i) {
            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string filename(len, '\0');
            in.read(&filename[0], len);
            filenamePool[i] = filename;
        }

        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        for (size_t i = 0; i < count; ++i) {
            size_t offset;
            in.read(reinterpret_cast<char*>(&offset), sizeof(offset));

            SourceLocation loc;
            in.read(reinterpret_cast<char*>(&loc.line), sizeof(loc.line));
            in.read(reinterpret_cast<char*>(&loc.column), sizeof(loc.column));

            uint32_t filenameIndex;
            in.read(reinterpret_cast<char*>(&filenameIndex), sizeof(filenameIndex));
            loc.filename = filenamePool[filenameIndex];

            sourceLocations[offset] = loc;
        }
    }

    void BytecodeProgram::readGlobalExceptionTable(std::istream& in) {
        size_t entryCount;
        in.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));
        readv::validateCount(entryCount, constants::security::MAX_EXCEPTION_TABLE_ENTRIES, "global exception table entries");

        for (size_t i = 0; i < entryCount; ++i) {
            size_t startIP, endIP, catchIP, finallyIP, catchVarSlot;
            uint32_t nestingLevel;

            in.read(reinterpret_cast<char*>(&startIP), sizeof(startIP));
            in.read(reinterpret_cast<char*>(&endIP), sizeof(endIP));
            in.read(reinterpret_cast<char*>(&catchIP), sizeof(catchIP));
            in.read(reinterpret_cast<char*>(&finallyIP), sizeof(finallyIP));
            in.read(reinterpret_cast<char*>(&nestingLevel), sizeof(nestingLevel));
            in.read(reinterpret_cast<char*>(&catchVarSlot), sizeof(catchVarSlot));

            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            readv::validateStringLen(len, "exception type");
            std::string exceptionType(len, '\0');
            in.read(&exceptionType[0], len);

            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            readv::validateStringLen(len, "catch var name");
            std::string catchVarName(len, '\0');
            in.read(&catchVarName[0], len);

            readv::validateExceptionEntry(startIP, endIP, catchIP, finallyIP,
                                          instructions.size(),
                                          "global");

            ExceptionTableEntry entry(startIP, endIP, catchIP, finallyIP, exceptionType, nestingLevel, catchVarName, catchVarSlot);
            globalExceptionTable.addEntry(entry);
        }
    }
}
