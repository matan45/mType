#include "LocalArrayFusionPass.hpp"
#include "../base/BytecodeOptimizationContext.hpp"
#include "../../bytecode/OpCode.hpp"

#include <cstdint>

namespace vm::optimization::passes
{
    using OpCode = vm::bytecode::OpCode;

    LocalArrayFusionPass::LocalArrayFusionPass()
        : BytecodePass("LocalArrayFusion", base::PassType::TRANSFORMATION)
    {
    }

    void LocalArrayFusionPass::optimize(bytecode::BytecodeProgram& program,
                                        base::BytecodeOptimizationContext& context)
    {
        const auto& functions = program.getFunctions();
        size_t totalInstructions = program.getInstructionCount();

        for (const auto& [name, meta] : functions)
        {
            size_t start = meta.startOffset;
            size_t end = start + meta.instructionCount;
            // Safety: cap end to actual instruction count (peephole may have removed instructions)
            if (end > totalInstructions)
                end = totalInstructions;

            for (size_t ip = start; ip < end; ++ip)
            {
                const auto& instr = program.getInstruction(ip);

                // Pattern 1: LOAD_LOCAL X, ARRAY_LENGTH → ARRAY_LENGTH_LOCAL X
                if (instr.opcode == OpCode::LOAD_LOCAL && ip + 1 < end)
                {
                    const auto& next = program.getInstruction(ip + 1);
                    if (next.opcode == OpCode::ARRAY_LENGTH)
                    {
                        uint64_t localIdx = instr.inlineOperands[0];
                        auto& mLoad = program.getMutableInstruction(ip);
                        mLoad.opcode = OpCode::NOP;
                        mLoad.clearOperands();
                        auto& mLen = program.getMutableInstruction(ip + 1);
                        mLen.opcode = OpCode::ARRAY_LENGTH_LOCAL;
                        mLen.setSingleOperand(localIdx);
                        recordTransformation();
                        context.setModified(true);
                        ip++; // skip the rewritten instruction
                        continue;
                    }
                }

                // Pattern 2: LOAD_LOCAL X, LOAD_LOCAL Y, ARRAY_GET_INT
                //           → LOAD_LOCAL Y, ARRAY_GET_INT_LOCAL X
                if (instr.opcode == OpCode::LOAD_LOCAL && ip + 2 < end)
                {
                    const auto& next1 = program.getInstruction(ip + 1);
                    const auto& next2 = program.getInstruction(ip + 2);
                    if (next1.opcode == OpCode::LOAD_LOCAL &&
                        next2.opcode == OpCode::ARRAY_GET_INT)
                    {
                        uint64_t arrLocal = instr.inlineOperands[0];
                        auto& mLoad = program.getMutableInstruction(ip);
                        mLoad.opcode = OpCode::NOP;
                        mLoad.clearOperands();
                        auto& mGet = program.getMutableInstruction(ip + 2);
                        mGet.opcode = OpCode::ARRAY_GET_INT_LOCAL;
                        mGet.setSingleOperand(arrLocal);
                        recordTransformation();
                        context.setModified(true);
                        ip += 2;
                        continue;
                    }
                }

                // Pattern 3: LOAD_LOCAL X, LOAD_LOCAL Y, <value>, ARRAY_SET_INT
                //           → LOAD_LOCAL Y, <value>, ARRAY_SET_INT_LOCAL X
                if (instr.opcode == OpCode::LOAD_LOCAL && ip + 3 < end)
                {
                    const auto& next1 = program.getInstruction(ip + 1);
                    const auto& next3 = program.getInstruction(ip + 3);
                    if (next1.opcode == OpCode::LOAD_LOCAL &&
                        next3.opcode == OpCode::ARRAY_SET_INT)
                    {
                        uint64_t arrLocal = instr.inlineOperands[0];
                        auto& mLoad = program.getMutableInstruction(ip);
                        mLoad.opcode = OpCode::NOP;
                        mLoad.clearOperands();
                        auto& mSet = program.getMutableInstruction(ip + 3);
                        mSet.opcode = OpCode::ARRAY_SET_INT_LOCAL;
                        mSet.setSingleOperand(arrLocal);
                        recordTransformation();
                        context.setModified(true);
                        ip += 3;
                        continue;
                    }
                }

            }
        }

        // Pattern 4 (MYT-303): ARRAY_GET, STORE_LOCAL → ARRAY_GET_ALIAS, STORE_LOCAL
        // The element is being aliased into a named local. Tell the executor
        // to demote SoA→heterogeneous so the local shares reference identity
        // with the array slot. Runs over the entire program (not per-function)
        // so module-level declarations outside any function are also rewritten.
        for (size_t ip = 0; ip + 1 < totalInstructions; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode != OpCode::ARRAY_GET) continue;
            const auto& next = program.getInstruction(ip + 1);
            if (next.opcode != OpCode::STORE_LOCAL) continue;
            program.getMutableInstruction(ip).opcode = OpCode::ARRAY_GET_ALIAS;
            recordTransformation();
            context.setModified(true);
        }
    }
}
