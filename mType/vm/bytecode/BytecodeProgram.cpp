#include "BytecodeProgram.hpp"
#include <cstddef>
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

    // MYT-198: lazy build of the fusion-unsafe set. Duplicates
    // JumpTargetTracker's logic in miniature to avoid a circular include from
    // bytecode/ into optimization/analysis/ (the tracker already depends on
    // BytecodeProgram). Keep the two in sync if new control-flow opcodes land.
    void BytecodeProgram::buildFusionUnsafeTargets() const {
        fusionUnsafeTargets.clear();

        for (size_t i = 0; i < instructions.size(); ++i) {
            const auto& instr = instructions[i];

            switch (instr.opcode) {
                case OpCode::JUMP:
                case OpCode::JUMP_IF_FALSE:
                case OpCode::JUMP_IF_TRUE:
                case OpCode::JUMP_IF_NULL:
                case OpCode::JUMP_BACK:
                case OpCode::JUMP_IF_FALSE_OR_POP:
                case OpCode::JUMP_IF_TRUE_OR_POP:
                    if (!instr.operands.empty()) {
                        fusionUnsafeTargets.insert(instr.operands[0]);
                    }
                    break;

                case OpCode::TRY_BEGIN:
                case OpCode::CATCH:
                case OpCode::FINALLY:
                    // The handler offset itself is a control-flow target, as
                    // is the jump operand (handler address) if present.
                    fusionUnsafeTargets.insert(i);
                    if (!instr.operands.empty()) {
                        fusionUnsafeTargets.insert(instr.operands[0]);
                    }
                    break;

                case OpCode::LOOP_START:
                    // Loop backs target LOOP_START.
                    fusionUnsafeTargets.insert(i);
                    break;

                default:
                    break;
            }
        }

        // Function entry points are reachable via CALL / CALL_FAST / CALL_METHOD
        // dispatch — the VM sets instructionPointer to (startOffset - 1) and the
        // loop advance lands on startOffset. Treat them as unsafe.
        for (const auto& [name, metadata] : functions) {
            fusionUnsafeTargets.insert(metadata.startOffset);
        }
        fusionUnsafeTargets.insert(entryPoint);

        fusionUnsafeTargetsBuilt = true;
    }

    bool BytecodeProgram::isFusionUnsafeTarget(size_t offset) const {
        if (!fusionUnsafeTargetsBuilt) {
            buildFusionUnsafeTargets();
        }
        return fusionUnsafeTargets.count(offset) > 0;
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

        invalidateFusionUnsafeTargets();
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

        invalidateFusionUnsafeTargets();
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

        invalidateFusionUnsafeTargets();
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

        // Compute allPrimitiveParams once at registration time. The set of
        // primitive type names matches the types that never require
        // convertLambdaArgumentsToInterfaces or auto-boxing at call sites.
        {
            auto& stored = functions[name];
            bool allPrim = true;
            for (const auto& t : stored.parameterTypes)
            {
                if (t != "int" && t != "float" && t != "bool" && t != "string"
                    && t != "void")
                {
                    allPrim = false;
                    break;
                }
            }
            stored.allPrimitiveParams = allPrim;
        }

        if (functionNameToIndex.find(name) == functionNameToIndex.end()) {
            size_t idx = functionIndexToName.size();
            functionIndexToName.push_back(name);
            functionNameToIndex[name] = idx;
        }

        // MYT-197: intern the name into the frame-name interner and backfill
        // its metadata pointer. Handle already present (interned at a prior
        // push site) just gets its meta slot filled in. std::unordered_map
        // does not invalidate pointers to values on rehash, so &functions[name]
        // is stable for the lifetime of the program.
        FunctionNameHandle handle = internFrameName(name);
        frameNameToMeta[handle.id] = &functions[name];
    }

    FunctionNameHandle BytecodeProgram::internFrameName(std::string_view name) const {
        auto it = frameNameToId.find(name);
        if (it != frameNameToId.end()) {
            return FunctionNameHandle{ it->second };
        }
        const uint32_t id = static_cast<uint32_t>(frameNameById.size());
        if (id == INVALID_FN_HANDLE.id) {
            throw std::runtime_error("BytecodeProgram: frame-name interner exhausted");
        }
        frameNameById.emplace_back(name);
        frameNameToMeta.push_back(nullptr);
        // Key the map on a string_view into the deque entry. deque::emplace_back
        // does not invalidate references to existing elements, so views from
        // earlier inserts remain valid.
        frameNameToId.emplace(std::string_view(frameNameById.back()), id);
        return FunctionNameHandle{ id };
    }

    const std::string& BytecodeProgram::getFrameName(FunctionNameHandle handle) const {
        // MYT-197: degrade gracefully on stale/cross-program handles. The hot
        // path that produces handles always interns them first, so invalid
        // ids should not flow here in well-formed programs — but returning a
        // stable empty string lets stack traces continue rather than throw
        // mid-format.
        static const std::string kEmpty;
        if (handle.id >= frameNameById.size()) {
            return kEmpty;
        }
        return frameNameById[handle.id];
    }

    const BytecodeProgram::FunctionMetadata* BytecodeProgram::getFunctionMeta(FunctionNameHandle handle) const {
        if (handle.id >= frameNameToMeta.size()) {
            return nullptr;
        }
        return frameNameToMeta[handle.id];
    }

    const BytecodeProgram::FunctionMetadata* BytecodeProgram::getFunction(const std::string& name) const {
        auto it = functions.find(name);
        return it != functions.end() ? &it->second : nullptr;
    }

    const std::unordered_map<std::string, BytecodeProgram::FunctionMetadata>& BytecodeProgram::getFunctions() const {
        return functions;
    }

    size_t BytecodeProgram::getFunctionIndex(const std::string& name) const {
        auto it = functionNameToIndex.find(name);
        return it != functionNameToIndex.end() ? it->second : SIZE_MAX;
    }

    const BytecodeProgram::FunctionMetadata* BytecodeProgram::getFunctionByIndex(size_t index) const {
        if (index >= functionIndexToName.size()) return nullptr;
        return getFunction(functionIndexToName[index]);
    }

    size_t BytecodeProgram::getFunctionCount() const {
        return functionIndexToName.size();
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

    // Bytecode format version — bumped whenever the opcode numbering or the
    // .mtc file layout changes. The deserializer rejects any other value with
    // a "recompile" diagnostic so a stale .mtc cannot silently misexecute
    // against a newer interpreter.
    //
    // History:
    //   8: MYT-202 compile-time fused superinstructions
    //   9: MYT-B1 dead lambda opcodes removed (LAMBDA_INVOKE / CLOSURE /
    //      LOAD_UPVALUE / STORE_UPVALUE / CLOSE_UPVALUE) — opcode numbering
    //      shifted, so prior .mtc files reference the wrong numeric op for
    //      every entry past the deletions.
    static constexpr uint32_t BYTECODE_FORMAT_VERSION = 9;

    void BytecodeProgram::serialize(std::ostream& out) const {
        // Write magic number
        uint32_t magic = 0x4D545950;  // "MTYP"
        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

        // Write version
        uint32_t version = BYTECODE_FORMAT_VERSION;
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

        // Write interface metadata
        writeInterfaces(out);

        // Write global exception table
        writeGlobalExceptionTable(out);

        // Write annotation declarations (MYT-108, .mtc v5+)
        writeAnnotationDeclarations(out);

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
        if (version != BYTECODE_FORMAT_VERSION) {
            throw std::runtime_error(
                "Bytecode file format version " + std::to_string(version) +
                " does not match the interpreter (expected " +
                std::to_string(BYTECODE_FORMAT_VERSION) + "). " +
                (version < BYTECODE_FORMAT_VERSION
                    ? "The .mtc was produced by an older compiler — recompile from source."
                    : "The .mtc was produced by a newer compiler — upgrade the interpreter."));
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

        // Read interface metadata
        program.readInterfaces(in);

        // Read global exception table
        program.readGlobalExceptionTable(in);

        // Read annotation declarations (MYT-108, .mtc v5+)
        program.readAnnotationDeclarations(in);

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
            // MYT-173 / MYT-194: runtime-only specialized opcodes must never
            // appear in a serialized bytecode stream. They're the product of
            // interpreter-side opcode rewriting, have no compile-time emitter,
            // and their auxiliary state (cachedMethodShape / cachedFieldShape
            // et al.) is not serialized — so accepting them here would leave
            // the VM in an inconsistent state. Reject explicitly rather than
            // relying on the CACHED handler to deopt on first dispatch.
            //
            // MYT-202 note: the compile-time superinstruction fusions
            // (LOAD_LOAD_ADD_INT et al.) are intentionally ABSENT from this
            // rejection list. They are emitted by the compile-time peephole
            // pass, carry their operands inside Instruction::operands (no
            // side-table state), and round-trip through .mtc normally.
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
                // Runtime-only: emitted only by trySpecializeBitwise after
                // observing monomorphic-INT operands. Never serialized.
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
            validateCount(opCount, constants::security::MAX_OPERANDS_PER_INSTR, "instruction operands");
            if (opCount > 0) {
                instructions[i].operands.resize(opCount);
                in.read(reinterpret_cast<char*>(instructions[i].operands.data()),
                       opCount * sizeof(uint64_t));
            }
        }
    }

    void BytecodeProgram::writeFunctions(std::ostream& out) const {
        // MYT-139: iterate in functionIndexToName order (stable insertion
        // order) rather than `functions` map order. `std::unordered_map`
        // iteration is non-deterministic, so writing in map order and
        // re-registering in read order re-shuffled function indices —
        // breaking CALL_FAST opcodes that bake function index at compile
        // time. See ticket MYT-139 for the symptom (e.g. renderDrawable
        // resolving to `tan` at runtime).
        size_t count = functionIndexToName.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& name : functionIndexToName) {
            auto it = functions.find(name);
            if (it == functions.end()) {
                throw std::runtime_error(
                    "BytecodeProgram::writeFunctions: index registered but no "
                    "metadata for function '" + name + "'");
            }
            const auto& func = it->second;
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

            // MYT-110: function-level annotations (v7+)
            writeAnnotationList(out, func.annotations);
            // MYT-110: per-parameter annotations (v7+)
            size_t fnParamAnnotCount = func.parameterNames.size();
            out.write(reinterpret_cast<const char*>(&fnParamAnnotCount), sizeof(fnParamAnnotCount));
            for (size_t i = 0; i < fnParamAnnotCount; ++i) {
                const std::vector<AnnotationData>& list =
                    (i < func.parameterAnnotations.size())
                    ? func.parameterAnnotations[i]
                    : std::vector<AnnotationData>{};
                writeAnnotationList(out, list);
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
            func.mangledName = name;
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

            // MYT-110: function-level annotations (v7+)
            readAnnotationList(in, func.annotations);
            // MYT-110: per-parameter annotations (v7+)
            size_t fnParamAnnotCount = 0;
            in.read(reinterpret_cast<char*>(&fnParamAnnotCount), sizeof(fnParamAnnotCount));
            if (!in) throw std::runtime_error("Malformed bytecode: failed to read function parameter-annotation count");
            validateCount(fnParamAnnotCount, constants::security::MAX_PARAMETERS_PER_FUNCTION, "function parameter-annotation count");
            func.parameterAnnotations.assign(fnParamAnnotCount, {});
            for (size_t j = 0; j < fnParamAnnotCount; ++j) {
                readAnnotationList(in, func.parameterAnnotations[j]);
            }

            registerFunction(name, func);
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

    void BytecodeProgram::addInterface(const InterfaceMetadata& interfaceMeta) {
        interfaces.push_back(interfaceMeta);
    }

    const std::vector<BytecodeProgram::InterfaceMetadata>& BytecodeProgram::getInterfaces() const {
        return interfaces;
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

    void BytecodeProgram::writeAnnotationList(std::ostream& out, const std::vector<AnnotationData>& list) {
        size_t count = list.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& annot : list) {
            BytecodeIOHelper::writeString(out, annot.name);
            int line   = annot.location.getLine();
            int column = annot.location.getColumn();
            out.write(reinterpret_cast<const char*>(&line),   sizeof(line));
            out.write(reinterpret_cast<const char*>(&column), sizeof(column));
            BytecodeIOHelper::writeString(out, annot.location.getFilename());
            size_t argCount = annot.typedArguments.size();
            out.write(reinterpret_cast<const char*>(&argCount), sizeof(argCount));
            for (const auto& arg : annot.typedArguments) {
                BytecodeIOHelper::writeString(out, arg.key);
                BytecodeIOHelper::writePrimitive(out, arg.valueType);
                BytecodeIOHelper::writePrimitive(out, arg.intVal);
                BytecodeIOHelper::writePrimitive(out, arg.floatVal);
                BytecodeIOHelper::writePrimitive(out, arg.boolVal);
                BytecodeIOHelper::writeString(out, arg.stringVal);
                BytecodeIOHelper::writeStringVector(out, arg.arrayVal);
            }
        }
    }

    void BytecodeProgram::readAnnotationList(std::istream& in, std::vector<AnnotationData>& list) {
        // Defensive caps: guards against corrupted/truncated .mtc files resizing
        // vectors to absurd sizes from garbage bytes. Real annotations rarely
        // exceed a few dozen per host, let alone the caps below.
        constexpr size_t MAX_ANNOTATIONS_PER_HOST = 4096;
        constexpr size_t MAX_ARGS_PER_ANNOTATION  = 256;

        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        if (!in) throw std::runtime_error("Malformed bytecode: failed to read annotation count");
        if (count > MAX_ANNOTATIONS_PER_HOST)
            throw std::runtime_error("Malformed bytecode: annotation count " +
                std::to_string(count) + " exceeds cap " +
                std::to_string(MAX_ANNOTATIONS_PER_HOST));
        list.resize(count);
        for (auto& annot : list) {
            annot.name = BytecodeIOHelper::readString(in);
            int line, column;
            in.read(reinterpret_cast<char*>(&line),   sizeof(line));
            in.read(reinterpret_cast<char*>(&column), sizeof(column));
            std::string filename = BytecodeIOHelper::readString(in);
            annot.location.setLine(line);
            annot.location.setColumn(column);
            annot.location.setFilename(filename);
            size_t argCount;
            in.read(reinterpret_cast<char*>(&argCount), sizeof(argCount));
            if (!in) throw std::runtime_error("Malformed bytecode: failed to read annotation arg count");
            if (argCount > MAX_ARGS_PER_ANNOTATION)
                throw std::runtime_error("Malformed bytecode: annotation arg count " +
                    std::to_string(argCount) + " exceeds cap " +
                    std::to_string(MAX_ARGS_PER_ANNOTATION));
            annot.typedArguments.resize(argCount);
            for (auto& arg : annot.typedArguments) {
                arg.key       = BytecodeIOHelper::readString(in);
                arg.valueType = BytecodeIOHelper::readPrimitive<uint8_t>(in);
                arg.intVal    = BytecodeIOHelper::readPrimitive<int64_t>(in);
                arg.floatVal  = BytecodeIOHelper::readPrimitive<double>(in);
                arg.boolVal   = BytecodeIOHelper::readPrimitive<bool>(in);
                arg.stringVal = BytecodeIOHelper::readString(in);
                arg.arrayVal  = BytecodeIOHelper::readStringVector(in);
            }
        }
    }

    void BytecodeProgram::writeFieldMetadata(std::ostream& out, const FieldMetadata& field) const {
        BytecodeIOHelper::writeString(out, field.name);
        BytecodeIOHelper::writeString(out, field.type);
        BytecodeIOHelper::writePrimitive(out, field.isStatic);
        BytecodeIOHelper::writePrimitive(out, field.isFinal);
        BytecodeIOHelper::writePrimitive(out, field.isPrivate);
        BytecodeIOHelper::writePrimitive(out, field.isProtected);
        writeAnnotationList(out, field.annotations);
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
        BytecodeIOHelper::writePrimitive(out, method.isAbstract);
        BytecodeIOHelper::writePrimitive(out, method.startOffset);
        writeAnnotationList(out, method.annotations);
        // MYT-110: per-parameter annotations, parallel to parameterNames.
        size_t paramCount = method.parameterNames.size();
        out.write(reinterpret_cast<const char*>(&paramCount), sizeof(paramCount));
        for (size_t i = 0; i < paramCount; ++i) {
            const std::vector<AnnotationData>& list =
                (i < method.parameterAnnotations.size())
                ? method.parameterAnnotations[i]
                : std::vector<AnnotationData>{};
            writeAnnotationList(out, list);
        }
    }

    void BytecodeProgram::writeConstructorMetadata(std::ostream& out, const ConstructorMetadata& ctor) const {
        BytecodeIOHelper::writeStringVector(out, ctor.parameterTypes);
        BytecodeIOHelper::writeStringVector(out, ctor.parameterNames);
        BytecodeIOHelper::writePrimitive(out, ctor.startOffset);
        writeAnnotationList(out, ctor.annotations);
        // MYT-110: per-parameter annotations, parallel to parameterNames.
        size_t paramCount = ctor.parameterNames.size();
        out.write(reinterpret_cast<const char*>(&paramCount), sizeof(paramCount));
        for (size_t i = 0; i < paramCount; ++i) {
            const std::vector<AnnotationData>& list =
                (i < ctor.parameterAnnotations.size())
                ? ctor.parameterAnnotations[i]
                : std::vector<AnnotationData>{};
            writeAnnotationList(out, list);
        }
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

        // Write annotations (MYT-108 typed-args format, via shared helper)
        writeAnnotationList(out, classMeta.annotations);

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

    void BytecodeProgram::writeAnnotationDeclarations(std::ostream& out) const {
        size_t declCount = annotationDeclarations.size();
        out.write(reinterpret_cast<const char*>(&declCount), sizeof(declCount));
        for (const auto& decl : annotationDeclarations) {
            BytecodeIOHelper::writeString(out, decl.name);
            size_t paramCount = decl.params.size();
            out.write(reinterpret_cast<const char*>(&paramCount), sizeof(paramCount));
            for (const auto& p : decl.params) {
                BytecodeIOHelper::writeString(out, p.name);
                BytecodeIOHelper::writePrimitive(out, p.declaredType);
                BytecodeIOHelper::writePrimitive(out, p.nullable);
                BytecodeIOHelper::writePrimitive(out, p.isArray);
                BytecodeIOHelper::writePrimitive(out, p.hasDefault);
                if (p.hasDefault) {
                    BytecodeIOHelper::writePrimitive(out, p.defaultInt);
                    BytecodeIOHelper::writePrimitive(out, p.defaultFloat);
                    BytecodeIOHelper::writePrimitive(out, p.defaultBool);
                    BytecodeIOHelper::writeString(out, p.defaultString);
                    BytecodeIOHelper::writeStringVector(out, p.defaultStringArray);
                }
            }
            // MYT-109 (.mtc v6+): meta-annotations on the annotation declaration.
            writeAnnotationList(out, decl.metaAnnotations);
        }
    }

    void BytecodeProgram::readAnnotationDeclarations(std::istream& in) {
        // Sanity cap — a single .mtc with more than a few thousand annotation
        // types is almost certainly corrupted.
        constexpr size_t MAX_ANNOTATION_DECLS = 4096;
        constexpr size_t MAX_PARAMS_PER_DECL  = 256;

        size_t declCount = BytecodeIOHelper::readPrimitive<size_t>(in);
        if (!in) throw std::runtime_error("Malformed bytecode: failed to read annotation decl count");
        if (declCount > MAX_ANNOTATION_DECLS)
            throw std::runtime_error("Malformed bytecode: annotation decl count " +
                std::to_string(declCount) + " exceeds cap " +
                std::to_string(MAX_ANNOTATION_DECLS));
        annotationDeclarations.resize(declCount);
        for (auto& decl : annotationDeclarations) {
            decl.name = BytecodeIOHelper::readString(in);
            size_t paramCount = BytecodeIOHelper::readPrimitive<size_t>(in);
            if (!in) throw std::runtime_error("Malformed bytecode: failed to read annotation param count");
            if (paramCount > MAX_PARAMS_PER_DECL)
                throw std::runtime_error("Malformed bytecode: annotation param count " +
                    std::to_string(paramCount) + " exceeds cap " +
                    std::to_string(MAX_PARAMS_PER_DECL));
            decl.params.resize(paramCount);
            for (auto& p : decl.params) {
                p.name         = BytecodeIOHelper::readString(in);
                p.declaredType = BytecodeIOHelper::readPrimitive<uint8_t>(in);
                p.nullable     = BytecodeIOHelper::readPrimitive<bool>(in);
                p.isArray      = BytecodeIOHelper::readPrimitive<bool>(in);
                p.hasDefault   = BytecodeIOHelper::readPrimitive<bool>(in);
                if (p.hasDefault) {
                    p.defaultInt         = BytecodeIOHelper::readPrimitive<int64_t>(in);
                    p.defaultFloat       = BytecodeIOHelper::readPrimitive<double>(in);
                    p.defaultBool        = BytecodeIOHelper::readPrimitive<bool>(in);
                    p.defaultString      = BytecodeIOHelper::readString(in);
                    p.defaultStringArray = BytecodeIOHelper::readStringVector(in);
                }
            }
            // MYT-109 (.mtc v6+): meta-annotations on the annotation declaration.
            readAnnotationList(in, decl.metaAnnotations);
        }
    }

    void BytecodeProgram::addAnnotationDeclaration(const AnnotationDeclData& declData) {
        annotationDeclarations.push_back(declData);
    }

    const std::vector<BytecodeProgram::AnnotationDeclData>&
        BytecodeProgram::getAnnotationDeclarations() const {
        return annotationDeclarations;
    }

    void BytecodeProgram::readFieldMetadata(std::istream& in, FieldMetadata& field) {
        field.name = BytecodeIOHelper::readString(in);
        field.type = BytecodeIOHelper::readString(in);
        field.isStatic = BytecodeIOHelper::readPrimitive<bool>(in);
        field.isFinal = BytecodeIOHelper::readPrimitive<bool>(in);
        field.isPrivate = BytecodeIOHelper::readPrimitive<bool>(in);
        field.isProtected = BytecodeIOHelper::readPrimitive<bool>(in);
        readAnnotationList(in, field.annotations);
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
        method.isAbstract = BytecodeIOHelper::readPrimitive<bool>(in);
        method.startOffset = BytecodeIOHelper::readPrimitive<size_t>(in);
        readAnnotationList(in, method.annotations);
        // MYT-110: per-parameter annotations
        size_t paramCount = 0;
        in.read(reinterpret_cast<char*>(&paramCount), sizeof(paramCount));
        if (!in) throw std::runtime_error("Malformed bytecode: failed to read method parameter-annotation count");
        validateCount(paramCount, constants::security::MAX_PARAMETERS_PER_FUNCTION, "method parameter-annotation count");
        method.parameterAnnotations.assign(paramCount, {});
        for (size_t i = 0; i < paramCount; ++i) {
            readAnnotationList(in, method.parameterAnnotations[i]);
        }
    }

    void BytecodeProgram::readConstructorMetadata(std::istream& in, ConstructorMetadata& ctor) {
        ctor.parameterTypes = BytecodeIOHelper::readStringVector(in);
        ctor.parameterNames = BytecodeIOHelper::readStringVector(in);
        ctor.startOffset = BytecodeIOHelper::readPrimitive<size_t>(in);
        readAnnotationList(in, ctor.annotations);
        // MYT-110: per-parameter annotations
        size_t paramCount = 0;
        in.read(reinterpret_cast<char*>(&paramCount), sizeof(paramCount));
        if (!in) throw std::runtime_error("Malformed bytecode: failed to read constructor parameter-annotation count");
        validateCount(paramCount, constants::security::MAX_PARAMETERS_PER_FUNCTION, "constructor parameter-annotation count");
        ctor.parameterAnnotations.assign(paramCount, {});
        for (size_t i = 0; i < paramCount; ++i) {
            readAnnotationList(in, ctor.parameterAnnotations[i]);
        }
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

        // Read annotations (MYT-108 typed-args format, via shared helper)
        readAnnotationList(in, classMeta.annotations);

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

    // === Interface Metadata Serialization ===

    void BytecodeProgram::writeInterfaceMethodSignature(std::ostream& out, const InterfaceMethodSignature& sig) const {
        BytecodeIOHelper::writeString(out, sig.name);
        BytecodeIOHelper::writeString(out, sig.returnType);
        BytecodeIOHelper::writeStringVector(out, sig.parameterTypes);
        BytecodeIOHelper::writeStringVector(out, sig.parameterNames);
        BytecodeIOHelper::writeStringVector(out, sig.genericTypeParameters);
    }

    void BytecodeProgram::readInterfaceMethodSignature(std::istream& in, InterfaceMethodSignature& sig) {
        sig.name = BytecodeIOHelper::readString(in);
        sig.returnType = BytecodeIOHelper::readString(in);
        sig.parameterTypes = BytecodeIOHelper::readStringVector(in);
        sig.parameterNames = BytecodeIOHelper::readStringVector(in);
        sig.genericTypeParameters = BytecodeIOHelper::readStringVector(in);
    }

    void BytecodeProgram::writeInterfaceMetadata(std::ostream& out, const InterfaceMetadata& meta) const {
        BytecodeIOHelper::writeString(out, meta.name);
        BytecodeIOHelper::writeStringVector(out, meta.genericParameters);
        BytecodeIOHelper::writeStringVector(out, meta.extendsInterfaces);
        BytecodeIOHelper::writePrimitive(out, meta.isFinal);

        size_t methodCount = meta.methods.size();
        out.write(reinterpret_cast<const char*>(&methodCount), sizeof(methodCount));
        for (const auto& method : meta.methods) {
            writeInterfaceMethodSignature(out, method);
        }
    }

    void BytecodeProgram::readInterfaceMetadata(std::istream& in, InterfaceMetadata& meta) {
        meta.name = BytecodeIOHelper::readString(in);
        meta.genericParameters = BytecodeIOHelper::readStringVector(in);
        meta.extendsInterfaces = BytecodeIOHelper::readStringVector(in);
        meta.isFinal = BytecodeIOHelper::readPrimitive<bool>(in);

        size_t methodCount;
        in.read(reinterpret_cast<char*>(&methodCount), sizeof(methodCount));
        meta.methods.resize(methodCount);
        for (auto& method : meta.methods) {
            readInterfaceMethodSignature(in, method);
        }
    }

    void BytecodeProgram::writeInterfaces(std::ostream& out) const {
        size_t count = interfaces.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& iface : interfaces) {
            writeInterfaceMetadata(out, iface);
        }
    }

    void BytecodeProgram::readInterfaces(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));

        if (count > constants::security::MAX_FUNCTION_COUNT) {
            throw std::runtime_error("Interface count exceeds security limit");
        }

        interfaces.resize(count);
        for (size_t i = 0; i < count; ++i) {
            readInterfaceMetadata(in, interfaces[i]);
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
