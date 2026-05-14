#include "LoopOptimizer.hpp"
#include <cstddef>
#include <algorithm>

namespace vm::runtime::optimization
{
    void LoopOptimizer::optimize()
    {
        // Step 1: Find all loops in the bytecode
        findLoops();

        // Step 2: Analyze and optimize each loop
        for (auto& loop : loops) {
            // Analyze which variables are modified
            analyzeModifiedVariables(loop);

            // Try to detect if this is a counted loop
            bool isCounted = analyzeCountedLoop(loop);

            // Find loop-invariant code
            findInvariantCode(loop);

            // Apply optimizations
            if (isCounted) {
                applyStrengthReduction(loop);
            }

            hoistInvariantCode(loop);
        }
    }

    void LoopOptimizer::findLoops()
    {
        const auto& instructions = program.getInstructions();

        std::vector<size_t> loopStarts;

        for (size_t i = 0; i < instructions.size(); ++i) {
            const auto& instr = instructions[i];

            if (instr.opcode == bytecode::OpCode::LOOP_START) {
                loopStarts.push_back(i);
            }
            else if (instr.opcode == bytecode::OpCode::LOOP_END) {
                if (!loopStarts.empty()) {
                    LoopInfo loop;
                    loop.startOffset = loopStarts.back();
                    loop.endOffset = i;

                    // Find JUMP_BACK instruction (marks end of loop body)
                    for (size_t j = loop.startOffset; j < loop.endOffset; ++j) {
                        if (instructions[j].opcode == bytecode::OpCode::JUMP_BACK) {
                            loop.loopBackJumpOffset = j;
                            break;
                        }
                    }

                    loops.push_back(loop);
                    loopStarts.pop_back();
                }
            }
        }
    }

    bool LoopOptimizer::analyzeCountedLoop(LoopInfo& loop)
    {
        // Pattern detection for: for (int i = start; i < limit; i += step)
        // This is a simplified heuristic - real compilers do more sophisticated analysis

        const auto& instructions = program.getInstructions();

        // Look for pattern:
        // 1. STORE_LOCAL (initialization)
        // 2. LOAD_LOCAL + LOAD_LOCAL + LT_INT (condition check)
        // 3. LOAD_LOCAL + PUSH_INT + ADD_INT + STORE_LOCAL (increment)

        size_t offset = loop.startOffset + 1;

        // Simple heuristic: if we see a pattern of LOAD_LOCAL, comparison, and ADD_INT
        // on the same local variable slot, it's likely a counted loop

        std::unordered_map<size_t, int> localVarOps; // Count operations per local slot

        for (size_t i = offset; i < loop.loopBackJumpOffset && i < instructions.size(); ++i) {
            const auto& instr = instructions[i];

            if (instr.opcode == bytecode::OpCode::LOAD_LOCAL ||
                instr.opcode == bytecode::OpCode::STORE_LOCAL) {
                if (instr.hasOperands()) {
                    localVarOps[instr.inlineOperands[0]]++;
                }
            }

            // Look for ADD_INT or SUB_INT followed by STORE_LOCAL (induction variable update)
            if ((instr.opcode == bytecode::OpCode::ADD_INT ||
                 instr.opcode == bytecode::OpCode::SUB_INT) &&
                i + 1 < instructions.size() &&
                instructions[i + 1].opcode == bytecode::OpCode::STORE_LOCAL) {

                loop.isCountedLoop = true;
                loop.inductionVarSlot = instructions[i + 1].inlineOperands[0];
                loop.stepValue = (instr.opcode == bytecode::OpCode::ADD_INT) ? 1 : -1;
                return true;
            }
        }

        return false;
    }

    void LoopOptimizer::analyzeModifiedVariables(LoopInfo& loop)
    {
        const auto& instructions = program.getInstructions();

        for (size_t i = loop.startOffset; i < loop.endOffset && i < instructions.size(); ++i) {
            const auto& instr = instructions[i];

            // Track local variable modifications
            if (instr.opcode == bytecode::OpCode::STORE_LOCAL) {
                if (instr.hasOperands()) {
                    loop.modifiedLocals.insert(instr.inlineOperands[0]);
                }
            }

            // Track global variable modifications
            if (instr.opcode == bytecode::OpCode::STORE_VAR ||
                instr.opcode == bytecode::OpCode::STORE_GLOBAL) {
                if (instr.hasOperands()) {
                    // Get variable name from constant pool
                    const std::string& varName = program.getConstantPool().getString(instr.inlineOperands[0]);
                    loop.modifiedGlobals.insert(varName);
                }
            }

            // Array stores also count as modifications
            if (instr.opcode == bytecode::OpCode::ARRAY_SET ||
                instr.opcode == bytecode::OpCode::ARRAY_SET_INT ||
                instr.opcode == bytecode::OpCode::ARRAY_SET_FIELD) {
                // Arrays are on the stack, but we should mark that array modification happens
                // This is conservative - we don't track which specific array
            }
        }
    }

    void LoopOptimizer::findInvariantCode(LoopInfo& loop)
    {
        const auto& instructions = program.getInstructions();

        // Find instructions that:
        // 1. Don't depend on loop-modified variables
        // 2. Have no side effects (safe to move)
        // 3. Produce a value used in the loop

        for (size_t i = loop.startOffset + 1; i < loop.loopBackJumpOffset && i < instructions.size(); ++i) {
            if (isInstructionInvariant(i, loop)) {
                loop.invariantInstructions.push_back(i);
            }
        }
    }

    bool LoopOptimizer::isInstructionInvariant(size_t offset, const LoopInfo& loop) const
    {
        const auto& instructions = program.getInstructions();
        const auto& instr = instructions[offset];

        // Instructions that are safe to hoist (pure operations with no side effects)
        switch (instr.opcode) {
            // Arithmetic operations (invariant if operands are invariant)
            case bytecode::OpCode::ADD:
            case bytecode::OpCode::SUB:
            case bytecode::OpCode::MUL:
            case bytecode::OpCode::DIV:
            case bytecode::OpCode::MOD:
            case bytecode::OpCode::ADD_INT:
            case bytecode::OpCode::SUB_INT:
            case bytecode::OpCode::MUL_INT:
            case bytecode::OpCode::DIV_INT:
            case bytecode::OpCode::ADD_FLOAT:
            case bytecode::OpCode::SUB_FLOAT:
            case bytecode::OpCode::MUL_FLOAT:
            case bytecode::OpCode::DIV_FLOAT:
                return true;

            // Constant loads are always invariant
            case bytecode::OpCode::PUSH_INT:
            case bytecode::OpCode::PUSH_FLOAT:
            case bytecode::OpCode::PUSH_STRING:
            case bytecode::OpCode::PUSH_BOOL:
            case bytecode::OpCode::PUSH_NULL:
                return true;

            // LOAD_LOCAL is invariant if the local is not modified in the loop
            case bytecode::OpCode::LOAD_LOCAL:
                if (instr.hasOperands()) {
                    size_t localSlot = instr.inlineOperands[0];
                    return loop.modifiedLocals.find(localSlot) == loop.modifiedLocals.end();
                }
                return false;

            // LOAD_GLOBAL is invariant if the global is not modified in the loop
            case bytecode::OpCode::LOAD_GLOBAL:
            case bytecode::OpCode::LOAD_VAR:
                if (instr.hasOperands()) {
                    const std::string& varName = program.getConstantPool().getString(instr.inlineOperands[0]);
                    return loop.modifiedGlobals.find(varName) == loop.modifiedGlobals.end();
                }
                return false;

            // Conservative: don't hoist array access, field access, or calls
            // (they may have side effects or depend on mutable state)
            case bytecode::OpCode::ARRAY_GET:
            case bytecode::OpCode::ARRAY_GET_ALIAS:
            case bytecode::OpCode::ARRAY_LENGTH:
            case bytecode::OpCode::GET_FIELD:
            case bytecode::OpCode::CALL:
            case bytecode::OpCode::CALL_METHOD:
                return false;

            default:
                return false;
        }
    }

    void LoopOptimizer::applyStrengthReduction(LoopInfo& loop)
    {
        // Strength reduction: replace expensive operations with cheaper ones
        // Example: i * constant -> accumulated addition

        // This is a placeholder for more sophisticated strength reduction
        // Real implementation would:
        // 1. Find multiplication by induction variable
        // 2. Replace with addition of accumulated value
        // 3. Insert initialization before loop

        // For now, we just mark that the loop is a counted loop
        // The VM can use this information at runtime
    }

    void LoopOptimizer::hoistInvariantCode(LoopInfo& loop)
    {
        // Loop Invariant Code Motion (LICM)
        // Move invariant instructions outside the loop

        if (loop.invariantInstructions.empty()) {
            return;
        }

        // For now, this is a conservative implementation
        // A full implementation would:
        // 1. Build a dependency graph
        // 2. Move instructions in topological order
        // 3. Update jump offsets
        // 4. Ensure correctness of moved code

        // Since bytecode modification is complex (need to update all jump offsets),
        // we'll mark these instructions for future optimization passes
        // or for JIT compilation hints
    }

} // namespace vm::runtime::optimization
