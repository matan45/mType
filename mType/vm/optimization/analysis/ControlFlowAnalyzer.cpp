#include "ControlFlowAnalyzer.hpp"
#include "../../bytecode/OpCode.hpp"
#include <queue>
#include <algorithm>
#include <iostream>

namespace vm::optimization::analysis
{
    using namespace bytecode;

    void ControlFlowAnalyzer::analyze(const BytecodeProgram& program)
    {
        clear();

        // Step 1: Analyze jump targets
        jumpTracker.analyze(program);

        // Step 2: Identify basic block boundaries
        identifyBasicBlocks(program);

        // Step 3: Build control flow graph edges
        buildControlFlowGraph(program);

        // Step 4: Detect unreachable code
        detectUnreachableCode(program);
    }

    bool ControlFlowAnalyzer::isJumpTarget(size_t offset) const
    {
        return jumpTracker.isJumpTarget(offset);
    }

    bool ControlFlowAnalyzer::isBasicBlockStart(size_t offset) const
    {
        return blockStarts.count(offset) > 0;
    }

    bool ControlFlowAnalyzer::isBasicBlockEnd(size_t offset) const
    {
        return blockEnds.count(offset) > 0;
    }

    const ControlFlowAnalyzer::BasicBlock* ControlFlowAnalyzer::getBasicBlock(size_t offset) const
    {
        auto it = offsetToBlockIndex.find(offset);
        if (it != offsetToBlockIndex.end())
        {
            return &blocks[it->second];
        }

        // If not found by exact offset, search for containing block
        for (const auto& block : blocks)
        {
            if (offset >= block.startOffset && offset <= block.endOffset)
            {
                return &block;
            }
        }

        return nullptr;
    }

    bool ControlFlowAnalyzer::canOptimizeRange(size_t start, size_t end) const
    {
        // Use jump tracker's check
        if (!jumpTracker.canOptimizeRange(start, end))
        {
            return false;
        }

        // Also check basic block boundaries
        const BasicBlock* startBlock = getBasicBlock(start);
        if (!startBlock)
        {
            return false;
        }

        // Ensure the entire range is within the same basic block
        if (end > startBlock->endOffset + 1)
        {
            return false;
        }

        return true;
    }

    bool ControlFlowAnalyzer::isUnreachable(size_t offset) const
    {
        return unreachableOffsets.count(offset) > 0;
    }

    void ControlFlowAnalyzer::clear()
    {
        blocks.clear();
        offsetToBlockIndex.clear();
        blockStarts.clear();
        blockEnds.clear();
        unreachableOffsets.clear();
        jumpTracker.clear();
    }

    void ControlFlowAnalyzer::identifyBasicBlocks(const BytecodeProgram& program)
    {
        const auto& instructions = program.getInstructions();
        if (instructions.empty())
        {
            return;
        }

        // Mark all basic block start points
        std::unordered_set<size_t> starts;

        // 1. Program entry point is a block start
        starts.insert(program.getEntryPoint());

        // 2. All jump targets are block starts
        for (size_t target : jumpTracker.getAllTargets())
        {
            starts.insert(target);
        }

        // 3. All function entry points are block starts
        for (size_t entry : jumpTracker.getFunctionEntryPoints())
        {
            starts.insert(entry);
        }

        // 4. Instructions immediately after block terminators are block starts
        for (size_t i = 0; i < instructions.size(); ++i)
        {
            if (isBlockTerminator(instructions[i].opcode))
            {
                if (i + 1 < instructions.size())
                {
                    starts.insert(i + 1);
                }
            }
        }

        // Convert to sorted vector for processing
        std::vector<size_t> sortedStarts(starts.begin(), starts.end());
        std::sort(sortedStarts.begin(), sortedStarts.end());

        // Create basic blocks
        for (size_t i = 0; i < sortedStarts.size(); ++i)
        {
            size_t start = sortedStarts[i];
            size_t end;

            if (i + 1 < sortedStarts.size())
            {
                // End is one before the next block start
                end = sortedStarts[i + 1] - 1;
            }
            else
            {
                // Last block extends to the end of the program
                end = instructions.size() - 1;
            }

            // Adjust end to be at a block terminator if possible
            for (size_t j = start; j <= end && j < instructions.size(); ++j)
            {
                if (isBlockTerminator(instructions[j].opcode))
                {
                    end = j;
                    break;
                }
            }

            blocks.emplace_back(start, end);
            blockStarts.insert(start);
            blockEnds.insert(end);

            // Map all offsets in this block to the block index
            for (size_t offset = start; offset <= end; ++offset)
            {
                offsetToBlockIndex[offset] = blocks.size() - 1;
            }
        }
    }

    void ControlFlowAnalyzer::buildControlFlowGraph(const BytecodeProgram& program)
    {
        const auto& instructions = program.getInstructions();

        for (size_t blockIdx = 0; blockIdx < blocks.size(); ++blockIdx)
        {
            const auto& block = blocks[blockIdx];
            size_t lastInstrOffset = block.endOffset;

            if (lastInstrOffset >= instructions.size())
            {
                continue;
            }

            const auto& lastInstr = instructions[lastInstrOffset];

            // Get branch targets (for jumps, calls, etc.)
            auto branchTargets = getBranchTargets(lastInstrOffset, lastInstr);
            for (size_t target : branchTargets)
            {
                size_t targetBlockIdx = findBlockIndex(target);
                if (targetBlockIdx != static_cast<size_t>(-1))
                {
                    addEdge(blockIdx, targetBlockIdx);
                }
            }

            // Get fall-through target (for non-unconditional control flow)
            if (!isBlockTerminator(lastInstr.opcode) ||
                lastInstr.opcode == OpCode::JUMP_IF_FALSE ||
                lastInstr.opcode == OpCode::JUMP_IF_TRUE ||
                lastInstr.opcode == OpCode::JUMP_IF_NULL ||
                lastInstr.opcode == OpCode::JUMP_IF_FALSE_OR_POP ||
                lastInstr.opcode == OpCode::JUMP_IF_TRUE_OR_POP)
            {
                size_t fallThrough = getFallThroughTarget(lastInstrOffset, program);
                if (fallThrough < instructions.size())
                {
                    size_t fallThroughBlockIdx = findBlockIndex(fallThrough);
                    if (fallThroughBlockIdx != static_cast<size_t>(-1))
                    {
                        addEdge(blockIdx, fallThroughBlockIdx);
                    }
                }
            }
        }
    }

    void ControlFlowAnalyzer::detectUnreachableCode(const BytecodeProgram& program)
    {
        // Mark all blocks as unreachable initially
        for (auto& block : blocks)
        {
            block.isReachable = false;
        }

        // BFS from entry point
        std::queue<size_t> worklist;
        std::unordered_set<size_t> visited;

        size_t entryBlockIdx = findBlockIndex(program.getEntryPoint());
        if (entryBlockIdx != static_cast<size_t>(-1))
        {
            worklist.push(entryBlockIdx);
            visited.insert(entryBlockIdx);
        }

        // Also add all function entry points
        const auto& functionEntries = jumpTracker.getFunctionEntryPoints();

        for (size_t entry : functionEntries)
        {
            size_t blockIdx = findBlockIndex(entry);
            if (blockIdx != static_cast<size_t>(-1) && visited.find(blockIdx) == visited.end())
            {
                worklist.push(blockIdx);
                visited.insert(blockIdx);
            }
        }

        // BFS traversal
        while (!worklist.empty())
        {
            size_t currentIdx = worklist.front();
            worklist.pop();

            blocks[currentIdx].isReachable = true;

            for (size_t successorIdx : blocks[currentIdx].successors)
            {
                if (visited.find(successorIdx) == visited.end())
                {
                    visited.insert(successorIdx);
                    worklist.push(successorIdx);
                }
            }
        }

        // Collect unreachable offsets
        for (const auto& block : blocks)
        {
            if (!block.isReachable)
            {
                for (size_t offset = block.startOffset; offset <= block.endOffset; ++offset)
                {
                    unreachableOffsets.insert(offset);
                }
            }
        }
    }

    void ControlFlowAnalyzer::addEdge(size_t fromBlockIdx, size_t toBlockIdx)
    {
        if (fromBlockIdx >= blocks.size() || toBlockIdx >= blocks.size())
        {
            return;
        }

        // Add successor
        if (std::find(blocks[fromBlockIdx].successors.begin(),
                     blocks[fromBlockIdx].successors.end(),
                     toBlockIdx) == blocks[fromBlockIdx].successors.end())
        {
            blocks[fromBlockIdx].successors.push_back(toBlockIdx);
        }

        // Add predecessor
        if (std::find(blocks[toBlockIdx].predecessors.begin(),
                     blocks[toBlockIdx].predecessors.end(),
                     fromBlockIdx) == blocks[toBlockIdx].predecessors.end())
        {
            blocks[toBlockIdx].predecessors.push_back(fromBlockIdx);
        }
    }

    size_t ControlFlowAnalyzer::findBlockIndex(size_t offset) const
    {
        auto it = offsetToBlockIndex.find(offset);
        if (it != offsetToBlockIndex.end())
        {
            return it->second;
        }
        return static_cast<size_t>(-1);
    }

    bool ControlFlowAnalyzer::isBlockTerminator(OpCode opcode)
    {
        switch (opcode)
        {
            case OpCode::JUMP:
            case OpCode::JUMP_BACK:
            case OpCode::JUMP_IF_FALSE:
            case OpCode::JUMP_IF_TRUE:
            case OpCode::JUMP_IF_NULL:
            case OpCode::JUMP_IF_FALSE_OR_POP:
            case OpCode::JUMP_IF_TRUE_OR_POP:
            case OpCode::RETURN:
            case OpCode::RETURN_VALUE:
            case OpCode::THROW:
            case OpCode::HALT:
                return true;

            default:
                return false;
        }
    }

    size_t ControlFlowAnalyzer::getFallThroughTarget(size_t offset, const BytecodeProgram& program) const
    {
        return offset + 1;
    }

    std::vector<size_t> ControlFlowAnalyzer::getBranchTargets(size_t offset,
                                                               const BytecodeProgram::Instruction& instr) const
    {
        std::vector<size_t> targets;

        switch (instr.opcode)
        {
            case OpCode::JUMP:
            case OpCode::JUMP_BACK:
            case OpCode::JUMP_IF_FALSE:
            case OpCode::JUMP_IF_TRUE:
            case OpCode::JUMP_IF_NULL:
            case OpCode::JUMP_IF_FALSE_OR_POP:
            case OpCode::JUMP_IF_TRUE_OR_POP:
                if (!instr.operands.empty())
                {
                    targets.push_back(instr.operands[0]);
                }
                break;

            // Other control flow instructions could be handled here
            default:
                break;
        }

        return targets;
    }

} // namespace vm::optimization::analysis
