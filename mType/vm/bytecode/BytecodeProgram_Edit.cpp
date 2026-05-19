#include "BytecodeProgram.hpp"
#include <stdexcept>

namespace vm::bytecode
{
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
                    if (instr.hasOperands()) {
                        fusionUnsafeTargets.insert(instr.inlineOperands[0]);
                    }
                    break;

                case OpCode::SWITCH_STRING:
                    if (instr.numOperands() >= 2) {
                        fusionUnsafeTargets.insert(instr.inlineOperands[0]);
                        const size_t caseCount = static_cast<size_t>(instr.inlineOperands[1]);
                        for (size_t c = 0; c < caseCount; ++c) {
                            const size_t targetOperand = 3 + c * 2;
                            if (targetOperand < instr.numOperands()) {
                                fusionUnsafeTargets.insert(
                                    static_cast<size_t>(instr.operandAt(targetOperand)));
                            }
                        }
                    }
                    break;

                case OpCode::TRY_BEGIN:
                case OpCode::CATCH:
                case OpCode::FINALLY:
                    // The handler offset itself is a control-flow target, as
                    // is the jump operand (handler address) if present.
                    fusionUnsafeTargets.insert(i);
                    if (instr.hasOperands()) {
                        fusionUnsafeTargets.insert(instr.inlineOperands[0]);
                    }
                    break;

                case OpCode::LOOP_START:
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

    void BytecodeProgram::replaceInstructions(size_t offset, size_t count, const std::vector<Instruction>& newInstructions) {
        if (offset + count > instructions.size()) {
            throw std::out_of_range("replaceInstructions: offset + count out of range");
        }

        // Capture source location BEFORE erasing the first instruction.
        SourceLocation firstInstrLocation;
        bool hasSourceLocation = false;
        if (!newInstructions.empty()) {
            auto srcLoc = getSourceLocation(offset);
            if (srcLoc != nullptr) {
                firstInstrLocation = *srcLoc;
                hasSourceLocation = true;
            }
        }

        for (size_t i = offset; i < offset + count; ++i) {
            sourceLocations.erase(i);
        }

        int delta = static_cast<int>(newInstructions.size()) - static_cast<int>(count);

        // Shift later source locations BEFORE mutating the instruction vector
        // so callers don't confuse old vs new offsets.
        if (delta != 0) {
            updateSourceLocationsAfterOffset(offset + count, delta);
        }

        instructions.erase(instructions.begin() + offset, instructions.begin() + offset + count);
        instructions.insert(instructions.begin() + offset, newInstructions.begin(), newInstructions.end());

        if (hasSourceLocation) {
            addSourceLocation(offset, firstInstrLocation.line, firstInstrLocation.column, firstInstrLocation.filename);
        }

        invalidateFusionUnsafeTargets();
    }

    void BytecodeProgram::removeInstructions(size_t offset, size_t count) {
        if (offset + count > instructions.size()) {
            throw std::out_of_range("removeInstructions: offset + count out of range");
        }

        for (size_t i = offset; i < offset + count; ++i) {
            sourceLocations.erase(i);
        }

        int delta = -static_cast<int>(count);
        updateSourceLocationsAfterOffset(offset + count, delta);

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
        // Validates every jump-target operand. Called after optimization; any
        // out-of-bounds target indicates a bug in the optimizer.
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
                    if (instr.hasOperands()) {
                        uint32_t target = static_cast<uint32_t>(instr.inlineOperands[0]);
                        if (target >= instructions.size()) {
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
                        throw std::runtime_error(
                            "Jump instruction has no operands. "
                            "Opcode: " + std::string(getOpCodeName(instr.opcode)) +
                            ", Instruction offset: " + std::to_string(i) +
                            ". This indicates malformed bytecode.");
                    }
                    break;

                case OpCode::SWITCH_STRING:
                {
                    if (instr.numOperands() < 2) {
                        throw std::runtime_error(
                            "SWITCH_STRING instruction has fewer than 2 operands. "
                            "Instruction offset: " + std::to_string(i) +
                            ". This indicates malformed bytecode.");
                    }
                    auto validateTarget = [&](uint64_t rawTarget) {
                        uint32_t target = static_cast<uint32_t>(rawTarget);
                        if (target >= instructions.size()) {
                            throw std::runtime_error(
                                "Invalid SWITCH_STRING jump target detected in updateAllJumpOffsets(). "
                                "Instruction offset: " + std::to_string(i) +
                                ", Jump target: " + std::to_string(target) +
                                ", Instruction count: " + std::to_string(instructions.size()) +
                                ". This indicates bytecode corruption during optimization.");
                        }
                    };
                    validateTarget(instr.inlineOperands[0]);
                    const size_t caseCount = static_cast<size_t>(instr.inlineOperands[1]);
                    for (size_t c = 0; c < caseCount; ++c) {
                        validateTarget(instr.operandAt(3 + c * 2));
                    }
                    break;
                }

                default:
                    break;
            }
        }

        invalidateFusionUnsafeTargets();
    }

    void BytecodeProgram::updateFunctionOffsets(size_t removalOffset, int delta) {
        for (auto& [name, metadata] : functions) {
            if (metadata.startOffset >= removalOffset) {
                int newOffset = static_cast<int>(metadata.startOffset) + delta;

                if (newOffset >= 0 && newOffset < static_cast<int>(instructions.size())) {
                    metadata.startOffset = static_cast<size_t>(newOffset);
                }
                else {
                    throw std::runtime_error(
                        "Function offset update error: Invalid offset for function '" + name + "'. "
                        "Old offset: " + std::to_string(metadata.startOffset) +
                        ", New offset: " + std::to_string(newOffset) +
                        ", Instruction count: " + std::to_string(instructions.size()) +
                        ". This indicates bytecode corruption during optimization.");
                }
            }
        }

        for (auto& classMeta : classes) {
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
        auto updateOffset = [removalOffset, delta, this](size_t& offset) {
            if (offset == SIZE_MAX) return;  // sentinel for absent handler
            if (offset >= removalOffset) {
                int newOffset = static_cast<int>(offset) + delta;
                if (newOffset >= 0 && newOffset < static_cast<int>(instructions.size())) {
                    offset = static_cast<size_t>(newOffset);
                }
                // Invalid offsets are tolerated here — they may resolve to a
                // valid target after a later optimization pass.
            }
        };

        for (auto& entry : globalExceptionTable.getEntriesMutable()) {
            updateOffset(entry.startIP);
            updateOffset(entry.endIP);
            updateOffset(entry.catchIP);
            updateOffset(entry.finallyIP);
        }

        for (auto& [name, metadata] : functions) {
            for (auto& entry : metadata.exceptionTable.getEntriesMutable()) {
                updateOffset(entry.startIP);
                updateOffset(entry.endIP);
                updateOffset(entry.catchIP);
                updateOffset(entry.finallyIP);
            }
        }
    }

    void BytecodeProgram::updateSourceLocationsAfterOffset(size_t afterOffset, int delta) {
        if (delta == 0) {
            return;
        }

        // Two-pass to avoid iterator invalidation while rewriting keys.
        std::vector<std::pair<size_t, SourceLocation>> locationsToUpdate;

        for (const auto& [offset, location] : sourceLocations) {
            if (offset >= afterOffset) {
                locationsToUpdate.emplace_back(offset, location);
            }
        }

        for (const auto& [oldOffset, _] : locationsToUpdate) {
            sourceLocations.erase(oldOffset);
        }

        for (const auto& [oldOffset, location] : locationsToUpdate) {
            size_t newOffset = static_cast<size_t>(static_cast<int>(oldOffset) + delta);
            sourceLocations[newOffset] = location;
        }
    }
}
