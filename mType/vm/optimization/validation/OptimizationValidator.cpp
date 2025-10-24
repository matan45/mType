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

                // Get stack effect for this instruction
                int effect = 0;
                if (!getStackEffect(instr, effect))
                {
                    // Unknown opcode - this is a validation error!
                    result.addError("Unknown opcode at offset " + std::to_string(offset) +
                                  ": " + std::string(getOpCodeName(instr.opcode)) +
                                  " (cannot calculate stack effect)");
                    hasError = true;
                    break;
                }

                // Apply stack effect
                stackDepth += effect;

                // Check for stack underflow
                if (stackDepth < 0)
                {
                    result.addError("Stack underflow at offset " + std::to_string(offset) +
                                  " (opcode: " + std::string(getOpCodeName(instr.opcode)) +
                                  ", block start: " + std::to_string(block.startOffset) + ")");
                    hasError = true;
                    break;
                }
            }
        }

        return result;
    }

    bool OptimizationValidator::getStackEffect(const BytecodeProgram::Instruction& instr, int& stackEffect) const
    {
        using OpCode = bytecode::OpCode;

        switch (instr.opcode)
        {
            // === Stack Operations (+1) ===
            case OpCode::PUSH_INT:
            case OpCode::PUSH_FLOAT:
            case OpCode::PUSH_STRING:
            case OpCode::PUSH_BOOL:
            case OpCode::PUSH_NULL:
                stackEffect = +1;
                return true;

            // === Stack Operations (0 or more) ===
            case OpCode::DUP:           // Duplicate top: [a] -> [a, a]
                stackEffect = +1;
                return true;

            case OpCode::DUP2:          // Duplicate top 2: [a, b] -> [a, b, a, b]
                stackEffect = +2;
                return true;

            case OpCode::SWAP:          // Swap top 2: [a, b] -> [b, a]
                stackEffect = 0;
                return true;

            case OpCode::POP:           // Pop one value
                stackEffect = -1;
                return true;

            // === Arithmetic Operations (consume 1, produce 1 = 0) ===
            case OpCode::NEG:           // Unary negation: [a] -> [-a]
            case OpCode::INC:           // Increment: [a] -> [a+1]
            case OpCode::DEC:           // Decrement: [a] -> [a-1]
                stackEffect = 0;
                return true;

            // === Arithmetic Operations (consume 2, produce 1 = -1) ===
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
            case OpCode::MOD:
            case OpCode::ADD_INT:
            case OpCode::SUB_INT:
            case OpCode::MUL_INT:
            case OpCode::DIV_INT:
            case OpCode::ADD_FLOAT:
            case OpCode::SUB_FLOAT:
            case OpCode::MUL_FLOAT:
            case OpCode::DIV_FLOAT:
                stackEffect = -1;  // [a, b] -> [result]
                return true;

            // === Comparison Operations (consume 2, produce 1 = -1) ===
            case OpCode::EQ:
            case OpCode::NE:
            case OpCode::LT:
            case OpCode::GT:
            case OpCode::LE:
            case OpCode::GE:
            case OpCode::EQ_INT:
            case OpCode::NE_INT:
            case OpCode::LT_INT:
            case OpCode::GT_INT:
                stackEffect = -1;  // [a, b] -> [bool]
                return true;

            // === Logical Operations ===
            case OpCode::AND:          // [a, b] -> [a && b]
            case OpCode::OR:           // [a, b] -> [a || b]
            case OpCode::XOR:          // [a, b] -> [a ^ b]
                stackEffect = -1;
                return true;

            case OpCode::NOT:          // [a] -> [!a]
                stackEffect = 0;
                return true;

            // === Variable Operations ===
            case OpCode::LOAD_VAR:      // Load variable -> push value
            case OpCode::LOAD_LOCAL:
            case OpCode::LOAD_GLOBAL:
            case OpCode::LOAD_UPVALUE:
                stackEffect = +1;
                return true;

            case OpCode::STORE_VAR:     // Pop value -> store in variable
            case OpCode::STORE_LOCAL:
            case OpCode::STORE_GLOBAL:
            case OpCode::STORE_UPVALUE:
                stackEffect = -1;
                return true;

            case OpCode::DECLARE_VAR:   // Declare variable (no stack effect)
            case OpCode::CLOSE_UPVALUE: // Close upvalue (no stack effect)
                stackEffect = 0;
                return true;

            // === Control Flow Operations ===
            case OpCode::JUMP:              // No stack effect
            case OpCode::JUMP_BACK:
                stackEffect = 0;
                return true;

            case OpCode::JUMP_IF_FALSE:     // [cond] -> [] (pops condition)
            case OpCode::JUMP_IF_TRUE:
            case OpCode::JUMP_IF_NULL:
                stackEffect = -1;
                return true;

            case OpCode::JUMP_IF_FALSE_OR_POP:  // Conditional: either pops or doesn't
            case OpCode::JUMP_IF_TRUE_OR_POP:   // Conservative: assume -1
                stackEffect = -1;
                return true;

            case OpCode::RETURN:            // Return (no value)
                stackEffect = 0;
                return true;

            case OpCode::RETURN_VALUE:      // [value] -> [] (pops return value)
                stackEffect = -1;
                return true;

            // === Function Operations ===
            case OpCode::CALL:              // [func, arg1, ..., argN] -> [result]
            case OpCode::CALL_NATIVE:       // Net effect: -(argCount) + 1 = -(argCount-1)
            case OpCode::CALL_FAST:         // But argCount is in operand, hard to calculate here
            case OpCode::TAIL_CALL:         // Conservative: assume -1 (most common case)
                if (!instr.operands.empty())
                {
                    // operand[0] = argument count
                    int argCount = static_cast<int>(instr.operands[0]);
                    stackEffect = -(argCount + 1) + 1;  // Pop func + args, push result
                }
                else
                {
                    stackEffect = -1;  // Conservative default
                }
                return true;

            case OpCode::CLOSURE:           // Create closure -> push closure value
                stackEffect = +1;
                return true;

            // === Object/Class Operations ===
            case OpCode::NEW_OBJECT:        // [args...] -> [object]
                if (!instr.operands.empty())
                {
                    // If operand contains constructor arg count
                    int argCount = static_cast<int>(instr.operands[0]);
                    stackEffect = -argCount + 1;
                }
                else
                {
                    stackEffect = +1;  // Conservative
                }
                return true;

            case OpCode::GET_FIELD:         // [object] -> [field_value]
            case OpCode::GET_FIELD_FAST:
                stackEffect = 0;  // Pop object, push field
                return true;

            case OpCode::SET_FIELD:         // [object, value] -> []
            case OpCode::SET_FIELD_FAST:
                stackEffect = -2;
                return true;

            case OpCode::GET_STATIC:        // [] -> [value]
                stackEffect = +1;
                return true;

            case OpCode::SET_STATIC:        // [value] -> []
                stackEffect = -1;
                return true;

            case OpCode::CALL_METHOD:       // [object, args...] -> [result]
            case OpCode::CALL_STATIC:
            case OpCode::INVOKE:
                if (!instr.operands.empty())
                {
                    int argCount = static_cast<int>(instr.operands[0]);
                    stackEffect = -(argCount + 1) + 1;  // Pop object + args, push result
                }
                else
                {
                    stackEffect = -1;  // Conservative
                }
                return true;

            case OpCode::SUPER_INVOKE:      // [this, args...] -> [result]
            case OpCode::SUPER_CONSTRUCTOR:
                stackEffect = -1;  // Conservative
                return true;

            case OpCode::GET_SUPER:         // [] -> [super_class]
                stackEffect = +1;
                return true;

            case OpCode::SUPER_GET_FIELD:   // [] -> [field_value]
                stackEffect = +1;
                return true;

            case OpCode::SUPER_SET_FIELD:   // [value] -> []
                stackEffect = -1;
                return true;

            // === Type Operations ===
            case OpCode::INSTANCEOF:        // [object] -> [bool]
            case OpCode::CHECK_TYPE:
            case OpCode::GET_TYPE:
                stackEffect = 0;  // Pop object, push result
                return true;

            case OpCode::CAST:              // [object] -> [casted_object]
            case OpCode::TYPE_CONVERT:
                stackEffect = 0;
                return true;

            // === Array Operations ===
            case OpCode::NEW_ARRAY:         // [size] -> [array]
                stackEffect = 0;
                return true;

            case OpCode::NEW_ARRAY_MULTI:   // [size1, size2, ...] -> [array]
                if (!instr.operands.empty())
                {
                    int dimensions = static_cast<int>(instr.operands[0]);
                    stackEffect = -dimensions + 1;
                }
                else
                {
                    stackEffect = 0;
                }
                return true;

            case OpCode::ARRAY_GET:         // [array, index] -> [element]
            case OpCode::ARRAY_GET_INT:
            case OpCode::ARRAY_GET_FIELD:
                stackEffect = -1;  // Pop array + index, push element
                return true;

            case OpCode::ARRAY_SET:         // [array, index, value] -> []
            case OpCode::ARRAY_SET_INT:
            case OpCode::ARRAY_SET_FIELD:
                stackEffect = -3;
                return true;

            case OpCode::ARRAY_LENGTH:      // [array] -> [length]
                stackEffect = 0;
                return true;

            case OpCode::ARRAY_LITERAL:     // [elem1, elem2, ...] -> [array]
                if (!instr.operands.empty())
                {
                    int elemCount = static_cast<int>(instr.operands[0]);
                    stackEffect = -elemCount + 1;
                }
                else
                {
                    stackEffect = +1;
                }
                return true;

            // === Lambda Operations ===
            case OpCode::LAMBDA:            // [captured_vars...] -> [lambda]
                // Complex: operands contain captured var count
                // For now, conservative: +1 (creates lambda on stack)
                stackEffect = +1;
                return true;

            case OpCode::LAMBDA_INVOKE:     // [lambda, args...] -> [result]
                stackEffect = -1;  // Conservative
                return true;

            case OpCode::CAPTURE_VARIABLE:  // Capture variable (implementation-dependent)
                stackEffect = 0;
                return true;

            // === Generic Type Operations ===
            case OpCode::BIND_GENERIC:
            case OpCode::RESOLVE_GENERIC:
            case OpCode::INSTANTIATE_GENERIC:
                stackEffect = 0;  // Metadata operations
                return true;

            // === Interface Operations ===
            case OpCode::IMPLEMENT_INTERFACE:
            case OpCode::CHECK_INTERFACE:
                stackEffect = 0;
                return true;

            case OpCode::LAMBDA_TO_INTERFACE:  // [lambda] -> [interface_impl]
                stackEffect = 0;
                return true;

            // === Exception Handling ===
            case OpCode::TRY_BEGIN:
            case OpCode::TRY_END:
            case OpCode::FINALLY:
                stackEffect = 0;  // Control flow markers
                return true;

            case OpCode::CATCH:             // [] -> [exception]
                stackEffect = +1;  // Pushes exception object
                return true;

            case OpCode::THROW:             // [exception] -> (unwinds stack)
                stackEffect = -1;  // Pops exception
                return true;

            // === Debugging & Profiling ===
            case OpCode::LINE:
            case OpCode::SOURCE_FILE:
            case OpCode::BREAKPOINT:
            case OpCode::PROFILE_ENTER:
            case OpCode::PROFILE_EXIT:
                stackEffect = 0;  // Metadata, no stack effect
                return true;

            // === Loop Optimization ===
            case OpCode::LOOP_START:
            case OpCode::LOOP_END:
                stackEffect = 0;  // Markers, no stack effect
                return true;

            // === Special Operations ===
            case OpCode::NOP:
                stackEffect = 0;
                return true;

            case OpCode::HALT:
                stackEffect = 0;
                return true;

            case OpCode::IMPORT:
                stackEffect = 0;  // Module loading
                return true;

            // === Async/Await Operations ===
            case OpCode::CREATE_PROMISE:    // [value] -> [promise]
                stackEffect = 0;
                return true;

            case OpCode::AWAIT:             // [promise] -> [value]
                stackEffect = 0;
                return true;

            case OpCode::PROMISE_RESOLVE:   // [promise, value] -> []
                stackEffect = -2;
                return true;

            // === Unknown Opcode ===
            default:
                return false;  // Unknown opcode - validation error!
        }
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
