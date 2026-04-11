#include "BytecodeProgram.hpp"
#include "BytecodeIOHelper.hpp"
#include "../../constants/SecurityConstants.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <iostream>
namespace vm::bytecode
{
    namespace
    {
        // Reject obviously malformed counts read from a bytecode stream before
        // they reach a vector::resize. Throws std::runtime_error so the caller
        // (deserialize) propagates a clear "bytecode rejected" message.
        inline void validateCount(size_t count, size_t max, const char* what) {
            if (count > max) {
                throw std::runtime_error(
                    std::string("Bytecode deserialization rejected: ") + what +
                    " count " + std::to_string(count) + " exceeds limit " + std::to_string(max));
            }
        }

        // Same idea, for individual serialized strings (function names, type
        // names, exception messages, ...). Caps at MAX_STRING_BYTES so an
        // attacker cannot allocate a multi-GiB std::string from a 16-byte
        // length prefix.
        inline void validateStringLen(size_t len, const char* what) {
            if (len > constants::security::MAX_STRING_BYTES) {
                throw std::runtime_error(
                    std::string("Bytecode deserialization rejected: ") + what +
                    " string length " + std::to_string(len) + " exceeds limit " +
                    std::to_string(constants::security::MAX_STRING_BYTES));
            }
        }

        // Sentinel for "no handler" used by the bytecode compiler. SIZE_MAX
        // means the catch / finally branch is absent and must not be range-
        // checked against the instruction array.
        constexpr size_t IP_NONE = static_cast<size_t>(-1);

        // Verify that an exception-table entry's instruction pointers fall
        // within the loaded instruction array. Skips the SIZE_MAX sentinel
        // used for absent catch / finally branches.
        inline void validateExceptionEntry(size_t startIP, size_t endIP,
                                           size_t catchIP, size_t finallyIP,
                                           size_t instructionCount,
                                           const char* context) {
            if (startIP >= endIP) {
                throw std::runtime_error(
                    std::string("Bytecode deserialization rejected: ") + context +
                    " exception entry has startIP " + std::to_string(startIP) +
                    " >= endIP " + std::to_string(endIP));
            }
            if (endIP > instructionCount) {
                throw std::runtime_error(
                    std::string("Bytecode deserialization rejected: ") + context +
                    " exception entry endIP " + std::to_string(endIP) +
                    " exceeds instruction count " + std::to_string(instructionCount));
            }
            if (catchIP != IP_NONE && catchIP >= instructionCount) {
                throw std::runtime_error(
                    std::string("Bytecode deserialization rejected: ") + context +
                    " exception entry catchIP " + std::to_string(catchIP) +
                    " out of range (instruction count " + std::to_string(instructionCount) + ")");
            }
            if (finallyIP != IP_NONE && finallyIP >= instructionCount) {
                throw std::runtime_error(
                    std::string("Bytecode deserialization rejected: ") + context +
                    " exception entry finallyIP " + std::to_string(finallyIP) +
                    " out of range (instruction count " + std::to_string(instructionCount) + ")");
            }
        }
    }

    // === Instruction Implementation ===

    BytecodeProgram::Instruction::Instruction() : opcode(OpCode::NOP) {}

    BytecodeProgram::Instruction::Instruction(OpCode op) : opcode(op) {}

    BytecodeProgram::Instruction::Instruction(OpCode op, uint64_t operand1)
        : opcode(op), operands{operand1} {}

    BytecodeProgram::Instruction::Instruction(OpCode op, uint64_t operand1, uint64_t operand2)
        : opcode(op), operands{operand1, operand2} {}

    BytecodeProgram::Instruction::Instruction(OpCode op, std::vector<uint64_t> ops)
        : opcode(op), operands(std::move(ops)) {}

    // === ConstantPool Implementation ===

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
            return it->second;  // Deduplicate strings
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

    // === BytecodeProgram Implementation ===

    BytecodeProgram::BytecodeProgram() : entryPoint(0) {}

    void BytecodeProgram::emit(OpCode opcode) {
        instructions.emplace_back(opcode);
    }

    void BytecodeProgram::emit(OpCode opcode, uint64_t operand) {
        instructions.emplace_back(opcode, operand);
    }

    void BytecodeProgram::emit(OpCode opcode, uint64_t operand1, uint64_t operand2) {
        instructions.emplace_back(opcode, operand1, operand2);
    }

    void BytecodeProgram::emit(OpCode opcode, const std::vector<uint64_t>& operands) {
        instructions.emplace_back(opcode, operands);
    }

    size_t BytecodeProgram::getCurrentOffset() const {
        return instructions.size();
    }

    void BytecodeProgram::patchJump(size_t instructionIndex, uint64_t targetOffset) {
        if (instructionIndex < instructions.size() && !instructions[instructionIndex].operands.empty()) {
            instructions[instructionIndex].operands[0] = targetOffset;
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

    // === Optimization Support ===

    void BytecodeProgram::replaceInstructions(size_t offset, size_t count, const std::vector<Instruction>& newInstructions) {
        if (offset + count > instructions.size()) {
            throw std::out_of_range("replaceInstructions: offset + count out of range");
        }

        // STEP 1: Capture source location BEFORE erasing (for the first instruction being replaced)
        SourceLocation firstInstrLocation;
        bool hasSourceLocation = false;
        if (!newInstructions.empty()) {
            auto srcLoc = getSourceLocation(offset);
            if (srcLoc != nullptr) {
                firstInstrLocation = *srcLoc;
                hasSourceLocation = true;
            }
        }

        // STEP 2: Clean up source locations for the range being removed [offset, offset+count)
        for (size_t i = offset; i < offset + count; ++i) {
            sourceLocations.erase(i);
        }

        // STEP 3: Calculate the delta for offset updates
        int delta = static_cast<int>(newInstructions.size()) - static_cast<int>(count);

        // STEP 4: Update all source locations after the replacement point BEFORE modifying instructions
        // This must happen before erase/insert to avoid confusion about which offsets to update
        if (delta != 0) {
            updateSourceLocationsAfterOffset(offset + count, delta);
        }

        // STEP 5: Erase the old instructions
        instructions.erase(instructions.begin() + offset, instructions.begin() + offset + count);

        // STEP 6: Insert the new instructions
        instructions.insert(instructions.begin() + offset, newInstructions.begin(), newInstructions.end());

        // STEP 7: Restore source location for the first new instruction if it existed
        if (hasSourceLocation) {
            addSourceLocation(offset, firstInstrLocation.line, firstInstrLocation.column, firstInstrLocation.filename);
        }
    }

    void BytecodeProgram::removeInstructions(size_t offset, size_t count) {
        if (offset + count > instructions.size()) {
            throw std::out_of_range("removeInstructions: offset + count out of range");
        }

        // STEP 1: Clean up source locations for the range being removed [offset, offset+count)
        for (size_t i = offset; i < offset + count; ++i) {
            sourceLocations.erase(i);
        }

        // STEP 2: Update all source locations after the removal point
        // Delta is negative since we're removing instructions
        int delta = -static_cast<int>(count);
        updateSourceLocationsAfterOffset(offset + count, delta);

        // STEP 3: Remove the instructions
        instructions.erase(instructions.begin() + offset, instructions.begin() + offset + count);
    }

    std::vector<BytecodeProgram::Instruction> BytecodeProgram::getInstructionRange(size_t start, size_t end) const {
        if (start > end || end > instructions.size()) {
            throw std::out_of_range("getInstructionRange: invalid range");
        }

        return std::vector<Instruction>(instructions.begin() + start, instructions.begin() + end);
    }

    void BytecodeProgram::updateAllJumpOffsets() {
        // Validate all jump targets are within bounds
        // This is called after optimization to ensure bytecode correctness
        // Any invalid jump target indicates a bug in the optimizer

        for (size_t i = 0; i < instructions.size(); ++i) {
            const auto& instr = instructions[i];

            // Check if this is a jump instruction
            switch (instr.opcode) {
                case OpCode::JUMP:
                case OpCode::JUMP_IF_FALSE:
                case OpCode::JUMP_IF_TRUE:
                case OpCode::JUMP_IF_NULL:
                case OpCode::JUMP_BACK:
                case OpCode::JUMP_IF_FALSE_OR_POP:
                case OpCode::JUMP_IF_TRUE_OR_POP:
                    // Jump instructions have target offset as first operand
                    // Ensure the target is within bounds
                    if (!instr.operands.empty()) {
                        uint32_t target = static_cast<uint32_t>(instr.operands[0]);
                        if (target >= instructions.size()) {
                            // CRITICAL: Invalid jump target detected - fail fast
                            throw std::runtime_error(
                                "Invalid jump target detected in updateAllJumpOffsets(). "
                                "Opcode: " + std::string(getOpCodeName(instr.opcode)) +
                                ", Instruction offset: " + std::to_string(i) +
                                ", Jump target: " + std::to_string(target) +
                                ", Instruction count: " + std::to_string(instructions.size()) +
                                ". This indicates bytecode corruption during optimization.");
                        }
                    }
                    else {
                        // Jump instruction with no operands - this is also an error
                        throw std::runtime_error(
                            "Jump instruction has no operands. "
                            "Opcode: " + std::string(getOpCodeName(instr.opcode)) +
                            ", Instruction offset: " + std::to_string(i) +
                            ". This indicates malformed bytecode.");
                    }
                    break;

                default:
                    break;
            }
        }
    }

    void BytecodeProgram::updateFunctionOffsets(size_t removalOffset, int delta) {
        // Update all function startOffset values that are after the removal point
        for (auto& [name, metadata] : functions) {
            if (metadata.startOffset >= removalOffset) {
                // Apply the delta (can be negative if instructions were removed)
                int newOffset = static_cast<int>(metadata.startOffset) + delta;

                // Ensure the new offset is valid
                if (newOffset >= 0 && newOffset < static_cast<int>(instructions.size())) {
                    metadata.startOffset = static_cast<size_t>(newOffset);
                }
                else {
                    // CRITICAL: Invalid function offset - fail fast
                    throw std::runtime_error(
                        "Function offset update error: Invalid offset for function '" + name + "'. "
                        "Old offset: " + std::to_string(metadata.startOffset) +
                        ", New offset: " + std::to_string(newOffset) +
                        ", Instruction count: " + std::to_string(instructions.size()) +
                        ". This indicates bytecode corruption during optimization.");
                }
            }
        }

        // Also update class method and constructor offsets
        for (auto& classMeta : classes) {
            // Update instance method offsets
            for (auto& method : classMeta.instanceMethods) {
                if (method.startOffset >= removalOffset) {
                    int newOffset = static_cast<int>(method.startOffset) + delta;
                    if (newOffset >= 0 && newOffset < static_cast<int>(instructions.size())) {
                        method.startOffset = static_cast<size_t>(newOffset);
                    }
                    else {
                        throw std::runtime_error(
                            "Method offset update error: Invalid offset for method '" +
                            classMeta.name + "::" + method.name + "'. "
                            "Old offset: " + std::to_string(method.startOffset) +
                            ", New offset: " + std::to_string(newOffset) +
                            ", Instruction count: " + std::to_string(instructions.size()));
                    }
                }
            }

            // Update static method offsets
            for (auto& method : classMeta.staticMethods) {
                if (method.startOffset >= removalOffset) {
                    int newOffset = static_cast<int>(method.startOffset) + delta;
                    if (newOffset >= 0 && newOffset < static_cast<int>(instructions.size())) {
                        method.startOffset = static_cast<size_t>(newOffset);
                    }
                    else {
                        throw std::runtime_error(
                            "Static method offset update error: Invalid offset for method '" +
                            classMeta.name + "::" + method.name + "'. "
                            "Old offset: " + std::to_string(method.startOffset) +
                            ", New offset: " + std::to_string(newOffset) +
                            ", Instruction count: " + std::to_string(instructions.size()));
                    }
                }
            }

            // Update constructor offsets
            for (auto& ctor : classMeta.constructors) {
                if (ctor.startOffset >= removalOffset) {
                    int newOffset = static_cast<int>(ctor.startOffset) + delta;
                    if (newOffset >= 0 && newOffset < static_cast<int>(instructions.size())) {
                        ctor.startOffset = static_cast<size_t>(newOffset);
                    }
                    else {
                        throw std::runtime_error(
                            "Constructor offset update error: Invalid offset for constructor of class '" +
                            classMeta.name + "'. "
                            "Old offset: " + std::to_string(ctor.startOffset) +
                            ", New offset: " + std::to_string(newOffset) +
                            ", Instruction count: " + std::to_string(instructions.size()));
                    }
                }
            }
        }
    }

    void BytecodeProgram::updateExceptionTableOffsets(size_t removalOffset, int delta) {
        // Helper lambda to update a single offset
        auto updateOffset = [removalOffset, delta, this](size_t& offset) {
            if (offset == SIZE_MAX) return;  // Skip sentinel values
            if (offset >= removalOffset) {
                int newOffset = static_cast<int>(offset) + delta;
                if (newOffset >= 0 && newOffset < static_cast<int>(instructions.size())) {
                    offset = static_cast<size_t>(newOffset);
                }
                // Note: We don't throw on invalid offsets for exception tables
                // because they might point to valid locations after different optimizations
            }
        };

        // Update global exception table
        for (auto& entry : globalExceptionTable.getEntriesMutable()) {
            updateOffset(entry.startIP);
            updateOffset(entry.endIP);
            updateOffset(entry.catchIP);
            updateOffset(entry.finallyIP);
        }

        // Update each function's exception table
        for (auto& [name, metadata] : functions) {
            for (auto& entry : metadata.exceptionTable.getEntriesMutable()) {
                updateOffset(entry.startIP);
                updateOffset(entry.endIP);
                updateOffset(entry.catchIP);
                updateOffset(entry.finallyIP);
            }
        }
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

    void BytecodeProgram::registerGlobalVariable(const GlobalVariableMetadata& metadata) {
        globalVariables.push_back(metadata);
    }

    const std::vector<BytecodeProgram::GlobalVariableMetadata>& BytecodeProgram::getGlobalVariables() const {
        return globalVariables;
    }

    ExceptionTable& BytecodeProgram::getGlobalExceptionTable() {
        return globalExceptionTable;
    }

    const ExceptionTable& BytecodeProgram::getGlobalExceptionTable() const {
        return globalExceptionTable;
    }

    void BytecodeProgram::addSourceLocation(size_t instructionOffset, uint32_t line, uint32_t column, const std::string& filename) {
        sourceLocations[instructionOffset] = {line, column, filename};
    }

    const BytecodeProgram::SourceLocation* BytecodeProgram::getSourceLocation(size_t instructionOffset) const {
        auto it = sourceLocations.find(instructionOffset);
        return it != sourceLocations.end() ? &it->second : nullptr;
    }

    void BytecodeProgram::updateSourceLocationsAfterOffset(size_t afterOffset, int delta) {
        if (delta == 0) {
            return;
        }

        // Collect all source locations that need to be updated
        // We need to do this in two passes to avoid iterator invalidation
        std::vector<std::pair<size_t, SourceLocation>> locationsToUpdate;

        for (const auto& [offset, location] : sourceLocations) {
            if (offset >= afterOffset) {
                locationsToUpdate.emplace_back(offset, location);
            }
        }

        // Remove old entries
        for (const auto& [oldOffset, _] : locationsToUpdate) {
            sourceLocations.erase(oldOffset);
        }

        // Add with updated offsets
        for (const auto& [oldOffset, location] : locationsToUpdate) {
            size_t newOffset = static_cast<size_t>(static_cast<int>(oldOffset) + delta);
            sourceLocations[newOffset] = location;
        }
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

            // Show instruction flags
            if (instr.flags & INSTR_FLAG_NONNULL_RECEIVER) {
                oss << " [nonnull]";
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
        uint32_t version = 3;  // Version 3: instruction flags for null check elimination
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

        // Write class metadata
        writeClasses(out);

        // Write global exception table
        writeGlobalExceptionTable(out);

        // Write source file path
        size_t len = sourceFilePath.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(sourceFilePath.data(), len);
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
        if (version < 3) {
            throw std::runtime_error(
                "Bytecode file uses an outdated format (version " + std::to_string(version) + "). "
                "Please recompile from source using the current compiler.");
        }
        if (version != 3) {
            throw std::runtime_error("Unsupported bytecode version: " + std::to_string(version));
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

        // Read class metadata
        program.readClasses(in);

        // Read global exception table
        program.readGlobalExceptionTable(in);

        // Read source file path
        size_t len;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string sourcePath(len, '\0');
        in.read(&sourcePath[0], len);
        program.sourceFilePath = sourcePath;

        return program;
    }

    // === Private Serialization Helpers ===

    void BytecodeProgram::writeConstantPool(std::ostream& out) const {
        // Write integers (64-bit)
        size_t count = constantPool.integers.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        out.write(reinterpret_cast<const char*>(constantPool.integers.data()),
                 count * sizeof(int64_t));

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
        // Read integers (64-bit)
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        validateCount(count, constants::security::MAX_CONSTANT_POOL_ENTRIES, "constant pool integers");
        constantPool.integers.resize(count);
        in.read(reinterpret_cast<char*>(constantPool.integers.data()),
               count * sizeof(int64_t));

        // Read floats
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        validateCount(count, constants::security::MAX_CONSTANT_POOL_ENTRIES, "constant pool floats");
        constantPool.floats.resize(count);
        in.read(reinterpret_cast<char*>(constantPool.floats.data()),
               count * sizeof(double));

        // Read strings
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        validateCount(count, constants::security::MAX_CONSTANT_POOL_ENTRIES, "constant pool strings");
        constantPool.strings.resize(count);
        for (size_t i = 0; i < count; ++i) {
            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            validateStringLen(len, "constant pool string");
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
            out.write(reinterpret_cast<const char*>(&instr.flags), sizeof(instr.flags));
            size_t opCount = instr.operands.size();
            out.write(reinterpret_cast<const char*>(&opCount), sizeof(opCount));
            if (opCount > 0) {
                out.write(reinterpret_cast<const char*>(instr.operands.data()),
                         opCount * sizeof(uint64_t));
            }
        }
    }

    void BytecodeProgram::readInstructions(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        validateCount(count, constants::security::MAX_INSTRUCTION_COUNT, "instructions");
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
            in.read(reinterpret_cast<char*>(&instructions[i].flags),
                   sizeof(instructions[i].flags));
            size_t opCount;
            in.read(reinterpret_cast<char*>(&opCount), sizeof(opCount));
            validateCount(opCount, constants::security::MAX_OPERANDS_PER_INSTR, "instruction operands");
            if (opCount > 0) {
                instructions[i].operands.resize(opCount);
                in.read(reinterpret_cast<char*>(instructions[i].operands.data()),
                       opCount * sizeof(uint64_t));
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

            // Write exception table
            const auto& exceptionTable = func.exceptionTable;
            size_t entryCount = exceptionTable.size();
            out.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));
            for (const auto& entry : exceptionTable.getEntries()) {
                out.write(reinterpret_cast<const char*>(&entry.startIP), sizeof(entry.startIP));
                out.write(reinterpret_cast<const char*>(&entry.endIP), sizeof(entry.endIP));
                out.write(reinterpret_cast<const char*>(&entry.catchIP), sizeof(entry.catchIP));
                out.write(reinterpret_cast<const char*>(&entry.finallyIP), sizeof(entry.finallyIP));
                out.write(reinterpret_cast<const char*>(&entry.nestingLevel), sizeof(entry.nestingLevel));
                out.write(reinterpret_cast<const char*>(&entry.catchVarSlot), sizeof(entry.catchVarSlot));

                // Write exception type string
                len = entry.exceptionType.size();
                out.write(reinterpret_cast<const char*>(&len), sizeof(len));
                out.write(entry.exceptionType.data(), len);

                // Write catch variable name string
                len = entry.catchVarName.size();
                out.write(reinterpret_cast<const char*>(&len), sizeof(len));
                out.write(entry.catchVarName.data(), len);
            }
        }
    }

    void BytecodeProgram::readFunctions(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        validateCount(count, constants::security::MAX_FUNCTION_COUNT, "functions");
        for (size_t i = 0; i < count; ++i) {
            // Read function name
            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            validateStringLen(len, "function name");
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

            // Native functions have no compiled body, so their offsets are
            // not constrained by the instruction array.
            if (!func.isNative) {
                if (func.startOffset > instructions.size() ||
                    func.instructionCount > instructions.size() ||
                    func.startOffset + func.instructionCount > instructions.size()) {
                    throw std::runtime_error(
                        "Bytecode deserialization rejected: function '" + name +
                        "' has out-of-range body [" + std::to_string(func.startOffset) +
                        " .. +" + std::to_string(func.instructionCount) +
                        "] (instruction count " + std::to_string(instructions.size()) + ")");
                }
            }

            // Read return type
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            validateStringLen(len, "function return type");
            std::string returnType(len, '\0');
            in.read(&returnType[0], len);
            func.returnType = returnType;

            // Read parameter names
            size_t paramCount;
            in.read(reinterpret_cast<char*>(&paramCount), sizeof(paramCount));
            validateCount(paramCount, constants::security::MAX_PARAMETERS_PER_FUNCTION, "function parameters");
            func.parameterNames.resize(paramCount);
            for (size_t j = 0; j < paramCount; ++j) {
                in.read(reinterpret_cast<char*>(&len), sizeof(len));
                validateStringLen(len, "function parameter name");
                std::string paramName(len, '\0');
                in.read(&paramName[0], len);
                func.parameterNames[j] = paramName;
            }

            // Read exception table
            size_t entryCount;
            in.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));
            validateCount(entryCount, constants::security::MAX_EXCEPTION_TABLE_ENTRIES, "function exception table entries");
            for (size_t j = 0; j < entryCount; ++j) {
                size_t startIP, endIP, catchIP, finallyIP, catchVarSlot;
                uint32_t nestingLevel;

                in.read(reinterpret_cast<char*>(&startIP), sizeof(startIP));
                in.read(reinterpret_cast<char*>(&endIP), sizeof(endIP));
                in.read(reinterpret_cast<char*>(&catchIP), sizeof(catchIP));
                in.read(reinterpret_cast<char*>(&finallyIP), sizeof(finallyIP));
                in.read(reinterpret_cast<char*>(&nestingLevel), sizeof(nestingLevel));
                in.read(reinterpret_cast<char*>(&catchVarSlot), sizeof(catchVarSlot));

                // Read exception type string
                in.read(reinterpret_cast<char*>(&len), sizeof(len));
                validateStringLen(len, "exception type");
                std::string exceptionType(len, '\0');
                in.read(&exceptionType[0], len);

                // Read catch variable name string
                in.read(reinterpret_cast<char*>(&len), sizeof(len));
                validateStringLen(len, "catch var name");
                std::string catchVarName(len, '\0');
                in.read(&catchVarName[0], len);

                // Validate IPs against the loaded instruction array.
                validateExceptionEntry(startIP, endIP, catchIP, finallyIP,
                                       instructions.size(),
                                       ("function '" + name + "'").c_str());

                // Add entry to function's exception table
                ExceptionTableEntry entry(startIP, endIP, catchIP, finallyIP, exceptionType, nestingLevel, catchVarName, catchVarSlot);
                func.exceptionTable.addEntry(entry);
            }

            functions[name] = func;
        }
    }

    void BytecodeProgram::writeSourceLocations(std::ostream& out) const {
        // Build filename string pool
        std::vector<std::string> filenamePool;
        std::unordered_map<std::string, uint32_t> filenameIndexMap;

        for (const auto& [offset, loc] : sourceLocations) {
            if (filenameIndexMap.find(loc.filename) == filenameIndexMap.end()) {
                filenameIndexMap[loc.filename] = static_cast<uint32_t>(filenamePool.size());
                filenamePool.push_back(loc.filename);
            }
        }

        // Write filename pool
        size_t poolSize = filenamePool.size();
        out.write(reinterpret_cast<const char*>(&poolSize), sizeof(poolSize));
        for (const auto& filename : filenamePool) {
            size_t len = filename.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(filename.data(), len);
        }

        // Write source locations with filename indices
        size_t count = sourceLocations.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& [offset, loc] : sourceLocations) {
            out.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
            out.write(reinterpret_cast<const char*>(&loc.line), sizeof(loc.line));
            out.write(reinterpret_cast<const char*>(&loc.column), sizeof(loc.column));

            uint32_t filenameIndex = filenameIndexMap[loc.filename];
            out.write(reinterpret_cast<const char*>(&filenameIndex), sizeof(filenameIndex));
        }
    }

    void BytecodeProgram::readSourceLocations(std::istream& in) {
        // Read filename pool
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

        // Read source locations with filename indices
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

    void BytecodeProgram::setSourceFilePath(const std::string& path) {
        sourceFilePath = path;
    }

    const std::string& BytecodeProgram::getSourceFilePath() const {
        return sourceFilePath;
    }

    void BytecodeProgram::registerClass(const ClassMetadata& classMeta) {
        classes.push_back(classMeta);
    }

    const std::vector<BytecodeProgram::ClassMetadata>& BytecodeProgram::getClasses() const {
        return classes;
    }

    bool BytecodeProgram::hasAsyncFunctions() const {
        // Check all functions (global functions, instance methods, static methods, constructors)
        // All are registered in the functions map with their fully qualified names
        for (const auto& pair : functions) {
            if (pair.second.isAsync) {
                return true;
            }
        }
        return false;
    }

    bool BytecodeProgram::hasAwaitInstructions() const {
        // Check if any instruction in the program is AWAIT
        // This tells us if the program actually uses await (not just declares async functions)
        for (const auto& instr : instructions) {
            if (instr.opcode == OpCode::AWAIT) {
                return true;
            }
        }
        return false;
    }

    // Helper functions now provided by BytecodeIOHelper utility class

    void BytecodeProgram::writeFieldMetadata(std::ostream& out, const FieldMetadata& field) const {
        BytecodeIOHelper::writeString(out, field.name);
        BytecodeIOHelper::writeString(out, field.type);
        BytecodeIOHelper::writePrimitive(out, field.isStatic);
        BytecodeIOHelper::writePrimitive(out, field.isFinal);
        BytecodeIOHelper::writePrimitive(out, field.isPrivate);
        BytecodeIOHelper::writePrimitive(out, field.isProtected);
    }

    void BytecodeProgram::writeMethodMetadata(std::ostream& out, const MethodMetadata& method) const {
        BytecodeIOHelper::writeString(out, method.name);
        BytecodeIOHelper::writeString(out, method.returnType);
        BytecodeIOHelper::writeStringVector(out, method.parameterTypes);
        BytecodeIOHelper::writeStringVector(out, method.parameterNames);
        BytecodeIOHelper::writePrimitive(out, method.isStatic);
        BytecodeIOHelper::writePrimitive(out, method.isFinal);
        BytecodeIOHelper::writePrimitive(out, method.isPrivate);
        BytecodeIOHelper::writePrimitive(out, method.isProtected);
        BytecodeIOHelper::writePrimitive(out, method.startOffset);
    }

    void BytecodeProgram::writeConstructorMetadata(std::ostream& out, const ConstructorMetadata& ctor) const {
        BytecodeIOHelper::writeStringVector(out, ctor.parameterTypes);
        BytecodeIOHelper::writeStringVector(out, ctor.parameterNames);
        BytecodeIOHelper::writePrimitive(out, ctor.startOffset);
    }

    void BytecodeProgram::writeClassMetadata(std::ostream& out, const ClassMetadata& classMeta) const {
        // Write class name
        BytecodeIOHelper::writeString(out, classMeta.name);

        // Write parent class name
        BytecodeIOHelper::writeString(out, classMeta.parentClassName);

        // Write implemented interfaces and generic parameters
        BytecodeIOHelper::writeStringVector(out, classMeta.implementedInterfaces);
        BytecodeIOHelper::writeStringVector(out, classMeta.genericParameters);

        // Write class modifier flags
        BytecodeIOHelper::writePrimitive(out, classMeta.isAbstract);
        BytecodeIOHelper::writePrimitive(out, classMeta.isFinal);
        BytecodeIOHelper::writePrimitive(out, classMeta.isValueClass);

        // Write annotations
        size_t annotCount = classMeta.annotations.size();
        out.write(reinterpret_cast<const char*>(&annotCount), sizeof(annotCount));
        for (const auto& annot : classMeta.annotations) {
            BytecodeIOHelper::writeString(out, annot.name);

            // Write source location using public getters
            int line = annot.location.getLine();
            int column = annot.location.getColumn();
            out.write(reinterpret_cast<const char*>(&line), sizeof(line));
            out.write(reinterpret_cast<const char*>(&column), sizeof(column));
            BytecodeIOHelper::writeString(out, annot.location.getFilename());

            // Write arguments
            size_t argCount = annot.arguments.size();
            out.write(reinterpret_cast<const char*>(&argCount), sizeof(argCount));
            for (const auto& [key, value] : annot.arguments) {
                BytecodeIOHelper::writeString(out, key);
                BytecodeIOHelper::writeString(out, value);
            }
        }

        // Write instance fields
        size_t fieldCount = classMeta.instanceFields.size();
        out.write(reinterpret_cast<const char*>(&fieldCount), sizeof(fieldCount));
        for (const auto& field : classMeta.instanceFields) {
            writeFieldMetadata(out, field);
        }

        // Write static fields
        fieldCount = classMeta.staticFields.size();
        out.write(reinterpret_cast<const char*>(&fieldCount), sizeof(fieldCount));
        for (const auto& field : classMeta.staticFields) {
            writeFieldMetadata(out, field);
        }

        // Write instance methods
        size_t methodCount = classMeta.instanceMethods.size();
        out.write(reinterpret_cast<const char*>(&methodCount), sizeof(methodCount));
        for (const auto& method : classMeta.instanceMethods) {
            writeMethodMetadata(out, method);
        }

        // Write static methods
        methodCount = classMeta.staticMethods.size();
        out.write(reinterpret_cast<const char*>(&methodCount), sizeof(methodCount));
        for (const auto& method : classMeta.staticMethods) {
            writeMethodMetadata(out, method);
        }

        // Write constructors
        size_t ctorCount = classMeta.constructors.size();
        out.write(reinterpret_cast<const char*>(&ctorCount), sizeof(ctorCount));
        for (const auto& ctor : classMeta.constructors) {
            writeConstructorMetadata(out, ctor);
        }
    }

    void BytecodeProgram::writeClasses(std::ostream& out) const {
        size_t count = classes.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& classMeta : classes) {
            writeClassMetadata(out, classMeta);
        }
    }

    void BytecodeProgram::readFieldMetadata(std::istream& in, FieldMetadata& field) {
        field.name = BytecodeIOHelper::readString(in);
        field.type = BytecodeIOHelper::readString(in);
        field.isStatic = BytecodeIOHelper::readPrimitive<bool>(in);
        field.isFinal = BytecodeIOHelper::readPrimitive<bool>(in);
        field.isPrivate = BytecodeIOHelper::readPrimitive<bool>(in);
        field.isProtected = BytecodeIOHelper::readPrimitive<bool>(in);
    }

    void BytecodeProgram::readMethodMetadata(std::istream& in, MethodMetadata& method) {
        method.name = BytecodeIOHelper::readString(in);
        method.returnType = BytecodeIOHelper::readString(in);
        method.parameterTypes = BytecodeIOHelper::readStringVector(in);
        method.parameterNames = BytecodeIOHelper::readStringVector(in);
        method.isStatic = BytecodeIOHelper::readPrimitive<bool>(in);
        method.isFinal = BytecodeIOHelper::readPrimitive<bool>(in);
        method.isPrivate = BytecodeIOHelper::readPrimitive<bool>(in);
        method.isProtected = BytecodeIOHelper::readPrimitive<bool>(in);
        method.startOffset = BytecodeIOHelper::readPrimitive<size_t>(in);
    }

    void BytecodeProgram::readConstructorMetadata(std::istream& in, ConstructorMetadata& ctor) {
        ctor.parameterTypes = BytecodeIOHelper::readStringVector(in);
        ctor.parameterNames = BytecodeIOHelper::readStringVector(in);
        ctor.startOffset = BytecodeIOHelper::readPrimitive<size_t>(in);
    }

    void BytecodeProgram::readClassMetadata(std::istream& in, ClassMetadata& classMeta) {
        // Read class name
        classMeta.name = BytecodeIOHelper::readString(in);

        // Read parent class name
        classMeta.parentClassName = BytecodeIOHelper::readString(in);

        // Read implemented interfaces and generic parameters
        classMeta.implementedInterfaces = BytecodeIOHelper::readStringVector(in);
        classMeta.genericParameters = BytecodeIOHelper::readStringVector(in);

        // Read class modifier flags
        classMeta.isAbstract = BytecodeIOHelper::readPrimitive<bool>(in);
        classMeta.isFinal = BytecodeIOHelper::readPrimitive<bool>(in);
        classMeta.isValueClass = BytecodeIOHelper::readPrimitive<bool>(in);

        // Read annotations
        size_t annotCount;
        in.read(reinterpret_cast<char*>(&annotCount), sizeof(annotCount));
        classMeta.annotations.resize(annotCount);
        for (auto& annot : classMeta.annotations) {
            annot.name = BytecodeIOHelper::readString(in);

            // Read source location using public setters
            int line, column;
            in.read(reinterpret_cast<char*>(&line), sizeof(line));
            in.read(reinterpret_cast<char*>(&column), sizeof(column));
            std::string filename = BytecodeIOHelper::readString(in);
            annot.location.setLine(line);
            annot.location.setColumn(column);
            annot.location.setFilename(filename);

            // Read arguments
            size_t argCount;
            in.read(reinterpret_cast<char*>(&argCount), sizeof(argCount));
            annot.arguments.resize(argCount);
            for (size_t i = 0; i < argCount; ++i) {
                std::string key = BytecodeIOHelper::readString(in);
                std::string value = BytecodeIOHelper::readString(in);
                annot.arguments[i] = {key, value};
            }
        }

        // Read instance fields
        size_t fieldCount;
        in.read(reinterpret_cast<char*>(&fieldCount), sizeof(fieldCount));
        classMeta.instanceFields.resize(fieldCount);
        for (auto& field : classMeta.instanceFields) {
            readFieldMetadata(in, field);
        }

        // Read static fields
        in.read(reinterpret_cast<char*>(&fieldCount), sizeof(fieldCount));
        classMeta.staticFields.resize(fieldCount);
        for (auto& field : classMeta.staticFields) {
            readFieldMetadata(in, field);
        }

        // Read instance methods
        size_t methodCount;
        in.read(reinterpret_cast<char*>(&methodCount), sizeof(methodCount));
        classMeta.instanceMethods.resize(methodCount);
        for (auto& method : classMeta.instanceMethods) {
            readMethodMetadata(in, method);
        }

        // Read static methods
        in.read(reinterpret_cast<char*>(&methodCount), sizeof(methodCount));
        classMeta.staticMethods.resize(methodCount);
        for (auto& method : classMeta.staticMethods) {
            readMethodMetadata(in, method);
        }

        // Read constructors
        size_t ctorCount;
        in.read(reinterpret_cast<char*>(&ctorCount), sizeof(ctorCount));
        classMeta.constructors.resize(ctorCount);
        for (auto& ctor : classMeta.constructors) {
            readConstructorMetadata(in, ctor);
        }
    }

    void BytecodeProgram::readClasses(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));

        classes.resize(count);
        for (size_t i = 0; i < count; ++i) {
            readClassMetadata(in, classes[i]);
        }
    }

    void BytecodeProgram::writeGlobalExceptionTable(std::ostream& out) const {
        // Write exception table entry count
        size_t entryCount = globalExceptionTable.size();
        out.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));

        // Write each entry
        for (const auto& entry : globalExceptionTable.getEntries()) {
            out.write(reinterpret_cast<const char*>(&entry.startIP), sizeof(entry.startIP));
            out.write(reinterpret_cast<const char*>(&entry.endIP), sizeof(entry.endIP));
            out.write(reinterpret_cast<const char*>(&entry.catchIP), sizeof(entry.catchIP));
            out.write(reinterpret_cast<const char*>(&entry.finallyIP), sizeof(entry.finallyIP));
            out.write(reinterpret_cast<const char*>(&entry.nestingLevel), sizeof(entry.nestingLevel));
            out.write(reinterpret_cast<const char*>(&entry.catchVarSlot), sizeof(entry.catchVarSlot));

            // Write exception type string
            size_t len = entry.exceptionType.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(entry.exceptionType.data(), len);

            // Write catch variable name string
            len = entry.catchVarName.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(entry.catchVarName.data(), len);
        }
    }

    void BytecodeProgram::readGlobalExceptionTable(std::istream& in) {
        // Read exception table entry count
        size_t entryCount;
        in.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));
        validateCount(entryCount, constants::security::MAX_EXCEPTION_TABLE_ENTRIES, "global exception table entries");

        // Read each entry
        for (size_t i = 0; i < entryCount; ++i) {
            size_t startIP, endIP, catchIP, finallyIP, catchVarSlot;
            uint32_t nestingLevel;

            in.read(reinterpret_cast<char*>(&startIP), sizeof(startIP));
            in.read(reinterpret_cast<char*>(&endIP), sizeof(endIP));
            in.read(reinterpret_cast<char*>(&catchIP), sizeof(catchIP));
            in.read(reinterpret_cast<char*>(&finallyIP), sizeof(finallyIP));
            in.read(reinterpret_cast<char*>(&nestingLevel), sizeof(nestingLevel));
            in.read(reinterpret_cast<char*>(&catchVarSlot), sizeof(catchVarSlot));

            // Read exception type string
            size_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            validateStringLen(len, "exception type");
            std::string exceptionType(len, '\0');
            in.read(&exceptionType[0], len);

            // Read catch variable name string
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            validateStringLen(len, "catch var name");
            std::string catchVarName(len, '\0');
            in.read(&catchVarName[0], len);

            // Validate IPs against the loaded instruction array.
            validateExceptionEntry(startIP, endIP, catchIP, finallyIP,
                                   instructions.size(),
                                   "global");

            // Add entry to global exception table
            ExceptionTableEntry entry(startIP, endIP, catchIP, finallyIP, exceptionType, nestingLevel, catchVarName, catchVarSlot);
            globalExceptionTable.addEntry(entry);
        }
    }
}
