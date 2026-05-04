#include "JumpTargetTracker.hpp"
#include <cstddef>
#include "../../bytecode/OpCode.hpp"

namespace vm::optimization::analysis
{
    using namespace bytecode;

    void JumpTargetTracker::analyze(const BytecodeProgram& program)
    {
        clear();

        scanJumpInstructions(program);
        scanFunctionEntries(program);
        scanExceptionHandlers(program);
        scanLoopMarkers(program);
    }

    bool JumpTargetTracker::isJumpTarget(size_t offset) const
    {
        return jumpTargets.count(offset) > 0;
    }

    bool JumpTargetTracker::isExceptionTarget(size_t offset) const
    {
        return exceptionTargets.count(offset) > 0;
    }

    bool JumpTargetTracker::canOptimizeRange(size_t start, size_t end) const
    {
        // Check if any interior positions (not the start) are jump targets
        for (size_t i = start + 1; i < end; ++i)
        {
            if (isJumpTarget(i) || isExceptionTarget(i) || functionEntries.count(i) > 0)
            {
                return false;  // Can't optimize across basic block boundaries
            }
        }
        return true;
    }

    void JumpTargetTracker::clear()
    {
        jumpTargets.clear();
        exceptionTargets.clear();
        functionEntries.clear();
        loopHeaders.clear();
    }

    void JumpTargetTracker::scanJumpInstructions(const BytecodeProgram& program)
    {
        const auto& instructions = program.getInstructions();

        for (size_t i = 0; i < instructions.size(); ++i)
        {
            const auto& instr = instructions[i];

            if (isJumpOpCode(instr.opcode) && !instr.operands.empty())
            {
                size_t target = calculateJumpTarget(i, instr);
                jumpTargets.insert(target);
            }
        }
    }

    void JumpTargetTracker::scanFunctionEntries(const BytecodeProgram& program)
    {
        const auto& functions = program.getFunctions();

        for (const auto& [name, metadata] : functions)
        {
            functionEntries.insert(metadata.startOffset);
        }

        // Also add the program entry point
        functionEntries.insert(program.getEntryPoint());
    }

    void JumpTargetTracker::scanExceptionHandlers(const BytecodeProgram& program)
    {
        const auto& instructions = program.getInstructions();

        for (size_t i = 0; i < instructions.size(); ++i)
        {
            const auto& instr = instructions[i];

            switch (instr.opcode)
            {
                case OpCode::TRY_BEGIN:
                case OpCode::CATCH:
                case OpCode::FINALLY:
                    // These mark exception handler boundaries
                    exceptionTargets.insert(i);
                    // If they have operands (jump to handler), mark those too
                    if (!instr.operands.empty())
                    {
                        exceptionTargets.insert(instr.operands[0]);
                    }
                    break;

                default:
                    break;
            }
        }
    }

    void JumpTargetTracker::scanLoopMarkers(const BytecodeProgram& program)
    {
        const auto& instructions = program.getInstructions();

        for (size_t i = 0; i < instructions.size(); ++i)
        {
            const auto& instr = instructions[i];

            if (instr.opcode == OpCode::LOOP_START)
            {
                loopHeaders.insert(i);
                jumpTargets.insert(i);  // Loop headers are also jump targets (for loop backs)
            }
        }
    }

    size_t JumpTargetTracker::calculateJumpTarget(size_t instructionOffset,
                                                   const BytecodeProgram::Instruction& instr) const
    {
        // For most jump instructions, the operand is the absolute target offset
        // JUMP_BACK might be relative in some implementations, but based on the codebase,
        // it appears to use absolute offsets
        if (!instr.operands.empty())
        {
            return instr.operands[0];
        }

        return instructionOffset;  // Fallback, shouldn't happen
    }

    bool JumpTargetTracker::isJumpOpCode(OpCode opcode)
    {
        switch (opcode)
        {
            case OpCode::JUMP:
            case OpCode::JUMP_IF_FALSE:
            case OpCode::JUMP_IF_TRUE:
            case OpCode::JUMP_IF_NULL:
            case OpCode::JUMP_BACK:
            case OpCode::JUMP_IF_FALSE_OR_POP:
            case OpCode::JUMP_IF_TRUE_OR_POP:
                return true;

            default:
                return false;
        }
    }

} // namespace vm::optimization::analysis
