#include "JumpThreadingPattern.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../analysis/ControlFlowAnalyzer.hpp"
#include <algorithm>

namespace vm::optimization::patterns
{
    using namespace bytecode;

    bool JumpThreadingPattern::matches(const BytecodeProgram& program,
                                       size_t offset,
                                       const analysis::ControlFlowAnalyzer& cfg) const
    {
        if (offset >= program.getInstructionCount())
        {
            return false;
        }

        const auto& instr = program.getInstruction(offset);

        // Only optimize jump instructions
        if (!isJumpInstruction(instr.opcode) || instr.operands.empty())
        {
            return false;
        }

        size_t targetOffset = instr.operands[0];

        // Check if target is within bounds
        if (targetOffset >= program.getInstructionCount())
        {
            return false;
        }

        // Check if the target is also a jump (potential for threading)
        const auto& targetInstr = program.getInstruction(targetOffset);

        // For unconditional jumps, thread if target is also a jump
        if (instr.opcode == OpCode::JUMP || instr.opcode == OpCode::JUMP_BACK)
        {
            if (isUnconditionalJump(program, targetOffset))
            {
                size_t finalTarget = getFinalJumpTarget(program, targetOffset);
                // Only optimize if we actually skip some jumps
                return finalTarget != targetOffset && isSafeToThread(program, offset, finalTarget);
            }
        }

        // For conditional jumps, only thread if target is unconditional jump
        if (instr.opcode == OpCode::JUMP_IF_FALSE ||
            instr.opcode == OpCode::JUMP_IF_TRUE ||
            instr.opcode == OpCode::JUMP_IF_NULL ||
            instr.opcode == OpCode::JUMP_IF_FALSE_OR_POP ||
            instr.opcode == OpCode::JUMP_IF_TRUE_OR_POP)
        {
            if (targetInstr.opcode == OpCode::JUMP && !targetInstr.operands.empty())
            {
                size_t finalTarget = getFinalJumpTarget(program, targetOffset);
                return finalTarget != targetOffset && isSafeToThread(program, offset, finalTarget);
            }
        }

        return false;
    }

    OptimizationPattern::Replacement JumpThreadingPattern::apply(const BytecodeProgram& program,
                                                                  size_t offset) const
    {
        const auto& instr = program.getInstruction(offset);
        size_t targetOffset = instr.operands[0];

        // Get the final target after threading through jumps
        size_t finalTarget = getFinalJumpTarget(program, targetOffset);

        // Create replacement instruction with updated target
        Replacement rep(1);
        rep.instructions.push_back(BytecodeProgram::Instruction(
            instr.opcode,
            static_cast<uint32_t>(finalTarget)
        ));

        return rep;
    }

    size_t JumpThreadingPattern::getFinalJumpTarget(const BytecodeProgram& program,
                                                     size_t startOffset,
                                                     int maxDepth) const
    {
        size_t currentOffset = startOffset;
        int depth = 0;

        // Use instruction count as max to ensure we follow entire chain
        // This prevents multiple passes for long jump chains
        int effectiveMaxDepth = std::max(maxDepth, static_cast<int>(program.getInstructionCount()));

        while (depth < effectiveMaxDepth && currentOffset < program.getInstructionCount())
        {
            const auto& instr = program.getInstruction(currentOffset);

            // If this is an unconditional jump, follow it
            if ((instr.opcode == OpCode::JUMP || instr.opcode == OpCode::JUMP_BACK) &&
                !instr.operands.empty())
            {
                size_t nextTarget = instr.operands[0];

                // Detect cycles (avoid infinite loops)
                if (nextTarget == currentOffset || nextTarget == startOffset)
                {
                    break;
                }

                currentOffset = nextTarget;
                depth++;
            }
            else
            {
                // Not a jump, this is our final target
                break;
            }
        }

        return currentOffset;
    }

    bool JumpThreadingPattern::isUnconditionalJump(const BytecodeProgram& program,
                                                    size_t offset) const
    {
        if (offset >= program.getInstructionCount())
        {
            return false;
        }

        const auto& instr = program.getInstruction(offset);
        return (instr.opcode == OpCode::JUMP || instr.opcode == OpCode::JUMP_BACK) &&
               !instr.operands.empty();
    }

    bool JumpThreadingPattern::isSafeToThread(const BytecodeProgram& program,
                                              size_t jumpOffset,
                                              size_t newTarget) const
    {
        // Don't thread if it creates a self-loop
        if (jumpOffset == newTarget)
        {
            return false;
        }

        // Don't thread if target is out of bounds
        if (newTarget >= program.getInstructionCount())
        {
            return false;
        }

        // Don't thread JUMP_BACK if it would change the direction
        const auto& instr = program.getInstruction(jumpOffset);
        if (instr.opcode == OpCode::JUMP_BACK)
        {
            // JUMP_BACK should jump backwards, ensure new target maintains this
            if (newTarget > jumpOffset)
            {
                return false;  // Would become a forward jump
            }
        }

        return true;
    }

} // namespace vm::optimization::patterns
