#include "OptimizationValidator.hpp"
#include "../../bytecode/OpCode.hpp"
#include <sstream>

namespace vm::optimization::validation
{
    using namespace bytecode;

    void OptimizationValidator::ValidationResult::addError(const std::string& error)
    {
        isValid = false;
        errors.push_back(error);
    }

    void OptimizationValidator::ValidationResult::addWarning(const std::string& warning)
    {
        warnings.push_back(warning);
    }

    std::string OptimizationValidator::ValidationResult::toString() const
    {
        std::ostringstream oss;

        if (isValid)
        {
            oss << "Validation PASSED\n";
        }
        else
        {
            oss << "Validation FAILED\n";
        }

        if (!errors.empty())
        {
            oss << "Errors:\n";
            for (const auto& error : errors)
            {
                oss << "  - " << error << "\n";
            }
        }

        if (!warnings.empty())
        {
            oss << "Warnings:\n";
            for (const auto& warning : warnings)
            {
                oss << "  - " << warning << "\n";
            }
        }

        return oss.str();
    }

    OptimizationValidator::ValidationResult OptimizationValidator::validate(const BytecodeProgram& program)
    {
        ValidationResult result;

        // Run all validation checks
        auto jumpResult = validateJumpTargets(program);
        if (!jumpResult.isValid)
        {
            result.isValid = false;
            result.errors.insert(result.errors.end(), jumpResult.errors.begin(), jumpResult.errors.end());
        }

        auto funcResult = validateFunctionMetadata(program);
        if (!funcResult.isValid)
        {
            result.isValid = false;
            result.errors.insert(result.errors.end(), funcResult.errors.begin(), funcResult.errors.end());
        }

        auto poolResult = validateConstantPool(program);
        if (!poolResult.isValid)
        {
            result.isValid = false;
            result.errors.insert(result.errors.end(), poolResult.errors.begin(), poolResult.errors.end());
        }

        auto stackResult = validateStackBalance(program);
        if (!stackResult.isValid)
        {
            // Stack balance is a warning, not an error
            result.warnings.insert(result.warnings.end(), stackResult.errors.begin(), stackResult.errors.end());
        }

        return result;
    }

    OptimizationValidator::ValidationResult OptimizationValidator::validateOptimization(
        const BytecodeProgram& original,
        const BytecodeProgram& optimized)
    {
        ValidationResult result;

        // Validate the optimized program itself
        auto validationResult = validate(optimized);
        if (!validationResult.isValid)
        {
            result = validationResult;
            result.addError("Optimized program failed basic validation");
            return result;
        }

        // Check if optimization didn't increase code size too much
        size_t originalSize = original.getInstructionCount();
        size_t optimizedSize = optimized.getInstructionCount();

        if (optimizedSize > originalSize)
        {
            result.addWarning("Optimization increased code size from " +
                            std::to_string(originalSize) + " to " +
                            std::to_string(optimizedSize) + " instructions");
        }

        // Check function count consistency
        if (original.getFunctions().size() != optimized.getFunctions().size())
        {
            result.addWarning("Function count changed during optimization");
        }

        // Check constant pool growth
        const auto& origPool = original.getConstantPool();
        const auto& optPool = optimized.getConstantPool();

        if (optPool.integers.size() > origPool.integers.size() * 2 ||
            optPool.floats.size() > origPool.floats.size() * 2 ||
            optPool.strings.size() > origPool.strings.size() * 2)
        {
            result.addWarning("Constant pool grew significantly during optimization");
        }

        return result;
    }

    OptimizationValidator::ValidationResult OptimizationValidator::validateJumpTargets(const BytecodeProgram& program)
    {
        ValidationResult result;
        const auto& instructions = program.getInstructions();
        size_t instructionCount = instructions.size();

        for (size_t i = 0; i < instructionCount; ++i)
        {
            const auto& instr = instructions[i];

            // Check jump instructions
            switch (instr.opcode)
            {
                case OpCode::JUMP:
                case OpCode::JUMP_IF_FALSE:
                case OpCode::JUMP_IF_TRUE:
                case OpCode::JUMP_IF_NULL:
                case OpCode::JUMP_BACK:
                case OpCode::JUMP_IF_FALSE_OR_POP:
                case OpCode::JUMP_IF_TRUE_OR_POP:
                    if (!instr.operands.empty())
                    {
                        uint32_t target = instr.operands[0];
                        if (target >= instructionCount)
                        {
                            result.addError("Jump at offset " + std::to_string(i) +
                                          " targets out-of-bounds offset " + std::to_string(target));
                        }
                    }
                    else
                    {
                        result.addError("Jump at offset " + std::to_string(i) + " missing target operand");
                    }
                    break;

                default:
                    break;
            }
        }

        return result;
    }

    OptimizationValidator::ValidationResult OptimizationValidator::validateFunctionMetadata(const BytecodeProgram& program)
    {
        ValidationResult result;
        const auto& functions = program.getFunctions();
        size_t instructionCount = program.getInstructionCount();

        for (const auto& [name, metadata] : functions)
        {
            // Check start offset
            if (metadata.startOffset >= instructionCount)
            {
                result.addError("Function '" + name + "' start offset " +
                              std::to_string(metadata.startOffset) + " is out of bounds");
            }

            // Check instruction count
            if (metadata.startOffset + metadata.instructionCount > instructionCount)
            {
                result.addError("Function '" + name + "' extends beyond program bounds");
            }

            // Validate parameter count matches parameter names
            if (metadata.parameterCount != metadata.parameterNames.size())
            {
                result.addWarning("Function '" + name + "' parameter count mismatch");
            }
        }

        return result;
    }

    OptimizationValidator::ValidationResult OptimizationValidator::validateConstantPool(const BytecodeProgram& program)
    {
        ValidationResult result;
        const auto& pool = program.getConstantPool();
        const auto& instructions = program.getInstructions();

        // Check if constant indices are valid
        for (size_t i = 0; i < instructions.size(); ++i)
        {
            const auto& instr = instructions[i];

            if (instr.opcode == OpCode::PUSH_INT && !instr.operands.empty())
            {
                uint32_t idx = instr.operands[0];
                if (idx >= pool.integers.size())
                {
                    result.addError("PUSH_INT at offset " + std::to_string(i) +
                                  " references invalid constant index " + std::to_string(idx));
                }
            }
            else if (instr.opcode == OpCode::PUSH_FLOAT && !instr.operands.empty())
            {
                uint32_t idx = instr.operands[0];
                if (idx >= pool.floats.size())
                {
                    result.addError("PUSH_FLOAT at offset " + std::to_string(i) +
                                  " references invalid constant index " + std::to_string(idx));
                }
            }
            else if (instr.opcode == OpCode::PUSH_STRING && !instr.operands.empty())
            {
                uint32_t idx = instr.operands[0];
                if (idx >= pool.strings.size())
                {
                    result.addError("PUSH_STRING at offset " + std::to_string(i) +
                                  " references invalid constant index " + std::to_string(idx));
                }
            }
        }

        return result;
    }

    OptimizationValidator::ValidationResult OptimizationValidator::validateStackBalance(const BytecodeProgram& program)
    {
        ValidationResult result;

        // Build CFG
        cfgAnalyzer.analyze(program);

        // For each basic block, check if stack is balanced
        // This is a simplified check - a full check would require data flow analysis
        const auto& blocks = cfgAnalyzer.getAllBlocks();

        for (const auto& block : blocks)
        {
            int stackDepth = 0;
            bool hasError = false;

            for (size_t offset = block.startOffset; offset <= block.endOffset && offset < program.getInstructionCount(); ++offset)
            {
                const auto& instr = program.getInstruction(offset);

                // Simplified stack effect calculation
                switch (instr.opcode)
                {
                    case OpCode::PUSH_INT:
                    case OpCode::PUSH_FLOAT:
                    case OpCode::PUSH_STRING:
                    case OpCode::PUSH_BOOL:
                    case OpCode::PUSH_NULL:
                    case OpCode::LOAD_LOCAL:
                    case OpCode::LOAD_GLOBAL:
                    case OpCode::DUP:
                        stackDepth++;
                        break;

                    case OpCode::POP:
                    case OpCode::STORE_LOCAL:
                    case OpCode::STORE_GLOBAL:
                    case OpCode::RETURN_VALUE:
                        stackDepth--;
                        if (stackDepth < 0)
                        {
                            result.addError("Stack underflow in basic block starting at " +
                                          std::to_string(block.startOffset));
                            hasError = true;
                        }
                        break;

                    case OpCode::ADD_INT:
                    case OpCode::SUB_INT:
                    case OpCode::MUL_INT:
                    case OpCode::DIV_INT:
                    case OpCode::ADD_FLOAT:
                    case OpCode::SUB_FLOAT:
                    case OpCode::MUL_FLOAT:
                    case OpCode::DIV_FLOAT:
                        stackDepth--;  // Consume 2, produce 1 = net -1
                        if (stackDepth < 0)
                        {
                            result.addError("Stack underflow in basic block starting at " +
                                          std::to_string(block.startOffset));
                            hasError = true;
                        }
                        break;

                    default:
                        // Skip unknown instructions
                        break;
                }

                if (hasError)
                {
                    break;
                }
            }
        }

        return result;
    }

    bool OptimizationValidator::areStructurallyEquivalent(const BytecodeProgram& p1,
                                                          const BytecodeProgram& p2)
    {
        // Compare function counts
        if (p1.getFunctions().size() != p2.getFunctions().size())
        {
            return false;
        }

        // Compare entry points
        if (p1.getEntryPoint() != p2.getEntryPoint())
        {
            return false;
        }

        // This is a simple structural check
        // A full equivalence check would require semantic analysis
        return true;
    }

} // namespace vm::optimization::validation
