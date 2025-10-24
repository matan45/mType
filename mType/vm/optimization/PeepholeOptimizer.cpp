#include "PeepholeOptimizer.hpp"
#include "patterns/ConstantFoldingPattern.hpp"
#include "patterns/DeadCodePattern.hpp"
#include "patterns/AlgebraicSimplificationPattern.hpp"
#include "patterns/JumpThreadingPattern.hpp"
#include "patterns/StrengthReductionPattern.hpp"
#include "patterns/LoadStoreOptimizationPattern.hpp"
#include "patterns/NullSequencePattern.hpp"
#include "patterns/StackOptimizationPattern.hpp"
#include "patterns/TypeSpecializationPattern.hpp"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iostream>

namespace vm::optimization
{
    // === Statistics Implementation ===

    void PeepholeOptimizer::Statistics::reset()
    {
        totalPasses = 0;
        instructionsEliminated = 0;
        constantsFolded = 0;
        deadCodeRemoved = 0;
        jumpsOptimized = 0;
        algebraicSimplifications = 0;
        strengthReductions = 0;
        loadStoreOptimizations = 0;
        typeSpecializations = 0;
        stackOptimizations = 0;
        nullSequencesRemoved = 0;
        patternApplications.clear();
        totalTimeMs = 0.0;
    }

    std::string PeepholeOptimizer::Statistics::toString() const
    {
        std::ostringstream oss;
        oss << "Peephole Optimization Statistics:\n";
        oss << "  Total Passes: " << totalPasses << "\n";
        oss << "  Instructions Eliminated: " << instructionsEliminated << "\n";
        oss << "  Constants Folded: " << constantsFolded << "\n";
        oss << "  Dead Code Removed: " << deadCodeRemoved << "\n";
        oss << "  Jumps Optimized: " << jumpsOptimized << "\n";
        oss << "  Algebraic Simplifications: " << algebraicSimplifications << "\n";
        oss << "  Strength Reductions: " << strengthReductions << "\n";
        oss << "  Load/Store Optimizations: " << loadStoreOptimizations << "\n";
        oss << "  Type Specializations: " << typeSpecializations << "\n";
        oss << "  Stack Optimizations: " << stackOptimizations << "\n";
        oss << "  Null Sequences Removed: " << nullSequencesRemoved << "\n";
        oss << "  Total Time: " << totalTimeMs << " ms\n";

        if (!patternApplications.empty())
        {
            oss << "  Pattern Applications:\n";
            for (const auto& [pattern, count] : patternApplications)
            {
                oss << "    " << pattern << ": " << count << "\n";
            }
        }

        return oss.str();
    }

    // === Config Implementation ===

    PeepholeOptimizer::Config PeepholeOptimizer::Config::forReleaseMode()
    {
        Config config;
        config.enableAggressiveOptimizations = true;
        config.maxPasses = 10;
        config.verboseOutput = false;
        config.validateAfterEachPass = false;
        config.timeoutMs = 5000.0;
        return config;
    }

    PeepholeOptimizer::Config PeepholeOptimizer::Config::forDebugMode()
    {
        Config config;
        config.enableAggressiveOptimizations = false;
        config.maxPasses = 3;
        config.verboseOutput = true;
        config.validateAfterEachPass = true;
        config.timeoutMs = 10000.0;
        return config;
    }

    // === PeepholeOptimizer Implementation ===

    PeepholeOptimizer::PeepholeOptimizer(const Config& cfg)
        : config(cfg)
    {
        resetStatistics();
    }

    bool PeepholeOptimizer::optimize(bytecode::BytecodeProgram& program)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        size_t initialInstructionCount = program.getInstructionCount();
        bool anyOptimizationsApplied = false;

        if (config.verboseOutput)
        {
            logOptimizationStart(program);
        }

        // Analyze the program once before optimization
        cfgAnalyzer.analyze(program);
        dataFlowAnalyzer.analyze(program);

        // Iteratively apply optimization passes
        for (size_t passNum = 0; passNum < config.maxPasses; ++passNum)
        {
            // Check timeout
            auto currentTime = std::chrono::high_resolution_clock::now();
            double elapsedMs = std::chrono::duration<double, std::milli>(currentTime - startTime).count();
            if (elapsedMs > config.timeoutMs)
            {
                if (config.verboseOutput)
                {
                    std::cout << "Peephole optimization timeout after " << elapsedMs << " ms\n";
                }
                break;
            }

            size_t optimizationsThisPass = runSinglePass(program);

            if (optimizationsThisPass == 0)
            {
                // Fixed point reached, no more optimizations possible
                break;
            }

            anyOptimizationsApplied = true;
            statistics.totalPasses++;

            // Re-analyze after modifications
            cfgAnalyzer.analyze(program);
            dataFlowAnalyzer.analyze(program);

            if (config.verboseOutput)
            {
                std::cout << "\n--- Pass " << (passNum + 1) << " completed: "
                    << optimizationsThisPass << " optimizations applied ---\n";
            }
        }

        // Update jump offsets after all modifications
        if (anyOptimizationsApplied)
        {
            updateJumpOffsets(program);
        }

        // Calculate statistics
        size_t finalInstructionCount = program.getInstructionCount();
        if (finalInstructionCount < initialInstructionCount)
        {
            statistics.instructionsEliminated = initialInstructionCount - finalInstructionCount;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        statistics.totalTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();


        logOptimizationEnd(program);
        std::cout << "\n" << statistics.toString() << "\n";


        return anyOptimizationsApplied;
    }

    void PeepholeOptimizer::registerPattern(std::unique_ptr<OptimizationPattern> pattern)
    {
        std::string name = pattern->getName();
        enabledPatterns[name] = true; // Enable by default
        patterns.push_back(std::move(pattern));
        sortPatternsByPriority();
    }

    void PeepholeOptimizer::enableOptimization(const std::string& name, bool enabled)
    {
        enabledPatterns[name] = enabled;
    }

    void PeepholeOptimizer::resetStatistics()
    {
        statistics.reset();
    }

    void PeepholeOptimizer::registerDefaultPatterns()
    {
        // Register patterns in priority order (high to low)
        // Higher priority patterns run first and enable subsequent optimizations

        // Priority 100: Constant Folding (enables other optimizations)
        registerPattern(std::make_unique<patterns::ConstantFoldingPattern>());

        // Priority 90: Dead Code Elimination
        registerPattern(std::make_unique<patterns::DeadCodePattern>());

        // Priority 70: Algebraic Simplification
        registerPattern(std::make_unique<patterns::AlgebraicSimplificationPattern>());

        // Priority 60: Jump Threading
        registerPattern(std::make_unique<patterns::JumpThreadingPattern>());

        // Priority 55: Load/Store Optimization
        registerPattern(std::make_unique<patterns::LoadStoreOptimizationPattern>());

        // Priority 50: Strength Reduction
        registerPattern(std::make_unique<patterns::StrengthReductionPattern>());

        // Priority 45: Null Sequence Elimination
        registerPattern(std::make_unique<patterns::NullSequencePattern>());

        // Priority 40: Stack Optimization
        registerPattern(std::make_unique<patterns::StackOptimizationPattern>());

        // Priority 30: Type Specialization (minor perf gain)
        registerPattern(std::make_unique<patterns::TypeSpecializationPattern>());

        // Patterns are automatically sorted by priority in registerPattern()
    }

    size_t PeepholeOptimizer::runSinglePass(bytecode::BytecodeProgram& program)
    {
        size_t optimizationsApplied = 0;
        size_t offset = 0;

        while (offset < program.getInstructionCount())
        {
            bool appliedAtOffset = false;

            // Try each pattern at this offset
            for (const auto& pattern : patterns)
            {
                // Check if pattern is enabled
                auto it = enabledPatterns.find(pattern->getName());
                if (it != enabledPatterns.end() && !it->second)
                {
                    continue; // Pattern disabled
                }

                if (tryApplyPattern(*pattern, program, offset))
                {
                    appliedAtOffset = true;
                    optimizationsApplied++;
                    recordPatternApplication(pattern->getName());

                    // Re-analyze after modification
                    // For performance, we could make this optional
                    if (config.validateAfterEachPass)
                    {
                        cfgAnalyzer.analyze(program);
                        dataFlowAnalyzer.analyze(program);
                    }

                    // Don't advance offset, re-check at same position
                    break;
                }
            }

            if (!appliedAtOffset)
            {
                // No pattern matched, move to next instruction
                offset++;
            }
        }

        return optimizationsApplied;
    }

    bool PeepholeOptimizer::tryApplyPattern(const OptimizationPattern& pattern,
                                            bytecode::BytecodeProgram& program,
                                            size_t offset)
    {
        // Check if pattern matches
        if (!pattern.matches(program, offset, cfgAnalyzer))
        {
            return false;
        }

        // Apply the pattern
        auto replacement = pattern.apply(program, offset);

        // If no optimization was actually applied (originalLength == 0), return false
        // This prevents infinite loops where a pattern matches but doesn't change anything
        if (replacement.originalLength == 0)
        {
            return false;
        }

        // Log the optimization if verbose
        if (config.verboseOutput)
        {
            logPatternMatch(pattern.getName(), offset, program, replacement);
        }

        // Calculate the change in instruction count
        int delta = static_cast<int>(replacement.instructions.size()) - static_cast<int>(replacement.originalLength);

        // Replace instructions in the program
        program.replaceInstructions(offset, replacement.originalLength, replacement.instructions);

        // CRITICAL: Update all jump offsets if instruction count changed
        if (delta != 0)
        {
            updateJumpOffsetsAfterReplacement(program, offset + replacement.originalLength, delta);

            // Update function metadata offsets
            // This ensures function entry points remain correct after instruction removal
            program.updateFunctionOffsets(offset + replacement.originalLength, delta);

            // Re-analyze CFG immediately after updating jumps and function offsets
            // This ensures subsequent patterns see the updated jump targets and function boundaries
            cfgAnalyzer.analyze(program);
            dataFlowAnalyzer.analyze(program);
        }

        return true;
    }

    void PeepholeOptimizer::updateJumpOffsets(bytecode::BytecodeProgram& program)
    {
        program.updateAllJumpOffsets();
    }

    void PeepholeOptimizer::updateJumpOffsetsAfterReplacement(bytecode::BytecodeProgram& program,
                                                               size_t replacementEndOffset,
                                                               int delta)
    {
        // Iterate through all instructions and update jump targets and lambda offsets
        for (size_t i = 0; i < program.getInstructionCount(); ++i)
        {
            auto& instr = const_cast<bytecode::BytecodeProgram::Instruction&>(program.getInstruction(i));

            // Check if this is a jump instruction
            bool isJump = false;
            switch (instr.opcode)
            {
                case bytecode::OpCode::JUMP:
                case bytecode::OpCode::JUMP_IF_FALSE:
                case bytecode::OpCode::JUMP_IF_TRUE:
                case bytecode::OpCode::JUMP_IF_NULL:
                case bytecode::OpCode::JUMP_BACK:
                case bytecode::OpCode::JUMP_IF_FALSE_OR_POP:
                case bytecode::OpCode::JUMP_IF_TRUE_OR_POP:
                    isJump = true;
                    break;
                default:
                    break;
            }

            if (isJump && !instr.operands.empty())
            {
                uint32_t target = instr.operands[0];

                // If the jump target is after the replacement point, adjust it
                if (target >= replacementEndOffset)
                {
                    // Apply the delta (can be negative if instructions were removed)
                    int newTarget = static_cast<int>(target) + delta;

                    // Ensure the new target is valid
                    if (newTarget >= 0 && newTarget < static_cast<int>(program.getInstructionCount()))
                    {
                        instr.operands[0] = static_cast<uint32_t>(newTarget);
                    }
                    else
                    {
                        std::cerr << "WARNING: Jump offset update resulted in invalid target: "
                                 << newTarget << " (instruction count: " << program.getInstructionCount() << ")" << std::endl;
                    }
                }
            }

            // CRITICAL: Also update LAMBDA instruction offsets
            // LAMBDA instruction format: operands[0] = lambda start offset
            if (instr.opcode == bytecode::OpCode::LAMBDA && !instr.operands.empty())
            {
                uint32_t lambdaOffset = instr.operands[0];

                // If the lambda start is after the replacement point, adjust it
                if (lambdaOffset >= replacementEndOffset)
                {
                    int newOffset = static_cast<int>(lambdaOffset) + delta;

                    if (newOffset >= 0 && newOffset < static_cast<int>(program.getInstructionCount()))
                    {
                        instr.operands[0] = static_cast<uint32_t>(newOffset);
                    }
                    else
                    {
                        std::cerr << "WARNING: Lambda offset update resulted in invalid target: "
                                 << newOffset << " (instruction count: " << program.getInstructionCount() << ")" << std::endl;
                    }
                }
            }
        }
    }

    void PeepholeOptimizer::sortPatternsByPriority()
    {
        std::sort(patterns.begin(), patterns.end(),
                  [](const std::unique_ptr<OptimizationPattern>& a,
                     const std::unique_ptr<OptimizationPattern>& b)
                  {
                      return a->getPriority() > b->getPriority();
                  });
    }

    void PeepholeOptimizer::recordPatternApplication(const std::string& patternName)
    {
        statistics.patternApplications[patternName]++;
    }

    // === Logging Methods ===

    void PeepholeOptimizer::logOptimizationStart(const bytecode::BytecodeProgram& program)
    {
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "  PEEPHOLE OPTIMIZATION STARTED\n";
        std::cout << "========================================\n";
        std::cout << "Initial instruction count: " << program.getInstructionCount() << "\n";
        std::cout << "Optimization level: Release\n";
        std::cout << "Max passes: " << config.maxPasses << "\n";
        std::cout << "Registered patterns: " << patterns.size() << "\n";
        std::cout << "========================================\n\n";
    }

    void PeepholeOptimizer::logOptimizationEnd(const bytecode::BytecodeProgram& program)
    {
        std::cout << "\n========================================\n";
        std::cout << "  PEEPHOLE OPTIMIZATION COMPLETED\n";
        std::cout << "========================================\n";
        std::cout << "Final instruction count: " << program.getInstructionCount() << "\n";
        std::cout << "Instructions eliminated: " << statistics.instructionsEliminated << "\n";
        std::cout << "Time elapsed: " << statistics.totalTimeMs << " ms\n";
        std::cout << "========================================\n";
    }

    void PeepholeOptimizer::logPatternMatch(const std::string& patternName,
                                            size_t offset,
                                            const bytecode::BytecodeProgram& program,
                                            const OptimizationPattern::Replacement& replacement)
    {
        // Skip logging if originalLength is 0 (no actual optimization)
        if (replacement.originalLength == 0)
        {
            return;
        }

        std::cout << "[" << patternName << "] at offset " << offset << ":\n";

        // Show original instructions
        std::cout << "  BEFORE: ";
        for (size_t i = 0; i < replacement.originalLength && (offset + i) < program.getInstructionCount(); ++i)
        {
            if (i > 0) std::cout << " | ";
            const auto& instr = program.getInstruction(offset + i);
            std::cout << formatInstruction(instr, offset + i);
        }
        std::cout << "\n";

        // Show replacement instructions
        std::cout << "  AFTER:  ";
        if (replacement.instructions.empty())
        {
            std::cout << "(removed)";
        }
        else
        {
            for (size_t i = 0; i < replacement.instructions.size(); ++i)
            {
                if (i > 0) std::cout << " | ";
                std::cout << formatInstruction(replacement.instructions[i], offset + i);
            }
        }
        std::cout << "\n";

        // Show the optimization benefit
        int instructionDiff = static_cast<int>(replacement.instructions.size()) - static_cast<int>(replacement.
            originalLength);
        if (instructionDiff < 0)
        {
            std::cout << "  SAVED:  " << (-instructionDiff) << " instruction(s)\n";
        }
        else if (instructionDiff == 0)
        {
            std::cout << "  TRANSFORM: Same size, improved efficiency\n";
        }
        else
        {
            std::cout << "  EXPANDED: +" << instructionDiff << " instruction(s) (specialized)\n";
        }

        std::cout << "\n";
    }

    std::string PeepholeOptimizer::formatInstruction(const bytecode::BytecodeProgram::Instruction& instr,
                                                     size_t offset) const
    {
        std::ostringstream oss;
        oss << opcodeToString(instr.opcode);

        // Add operands if present
        if (!instr.operands.empty())
        {
            oss << "(";
            for (size_t i = 0; i < instr.operands.size(); ++i)
            {
                if (i > 0) oss << ", ";
                oss << instr.operands[i];
            }
            oss << ")";
        }

        return oss.str();
    }

    std::string PeepholeOptimizer::opcodeToString(bytecode::OpCode opcode) const
    {
        using namespace bytecode;

        switch (opcode)
        {
        // Constants
        case OpCode::PUSH_INT: return "PUSH_INT";
        case OpCode::PUSH_FLOAT: return "PUSH_FLOAT";
        case OpCode::PUSH_STRING: return "PUSH_STRING";
        case OpCode::PUSH_BOOL: return "PUSH_BOOL";
        case OpCode::PUSH_NULL: return "PUSH_NULL";

        // Arithmetic - Integer
        case OpCode::ADD_INT: return "ADD_INT";
        case OpCode::SUB_INT: return "SUB_INT";
        case OpCode::MUL_INT: return "MUL_INT";
        case OpCode::DIV_INT: return "DIV_INT";
        case OpCode::MOD: return "MOD";

        // Arithmetic - Float
        case OpCode::ADD_FLOAT: return "ADD_FLOAT";
        case OpCode::SUB_FLOAT: return "SUB_FLOAT";
        case OpCode::MUL_FLOAT: return "MUL_FLOAT";
        case OpCode::DIV_FLOAT: return "DIV_FLOAT";

        // Arithmetic - Generic
        case OpCode::ADD: return "ADD";
        case OpCode::SUB: return "SUB";
        case OpCode::MUL: return "MUL";
        case OpCode::DIV: return "DIV";

        // Unary
        case OpCode::NEG: return "NEG";
        case OpCode::NOT: return "NOT";
        case OpCode::INC: return "INC";
        case OpCode::DEC: return "DEC";

        // Comparison - Integer
        case OpCode::EQ_INT: return "EQ_INT";
        case OpCode::NE_INT: return "NE_INT";
        case OpCode::LT_INT: return "LT_INT";
        case OpCode::GT_INT: return "GT_INT";

        // Comparison - Generic
        case OpCode::EQ: return "EQ";
        case OpCode::NE: return "NE";
        case OpCode::LT: return "LT";
        case OpCode::GT: return "GT";
        case OpCode::LE: return "LE";
        case OpCode::GE: return "GE";

        // Logical
        case OpCode::AND: return "AND";
        case OpCode::OR: return "OR";
        case OpCode::XOR: return "XOR";

        // Stack
        case OpCode::POP: return "POP";
        case OpCode::DUP: return "DUP";
        case OpCode::DUP2: return "DUP2";
        case OpCode::SWAP: return "SWAP";

        // Variables
        case OpCode::LOAD_VAR: return "LOAD_VAR";
        case OpCode::STORE_VAR: return "STORE_VAR";
        case OpCode::LOAD_LOCAL: return "LOAD_LOCAL";
        case OpCode::STORE_LOCAL: return "STORE_LOCAL";
        case OpCode::LOAD_GLOBAL: return "LOAD_GLOBAL";
        case OpCode::STORE_GLOBAL: return "STORE_GLOBAL";
        case OpCode::LOAD_UPVALUE: return "LOAD_UPVALUE";
        case OpCode::STORE_UPVALUE: return "STORE_UPVALUE";

        // Control Flow
        case OpCode::JUMP: return "JUMP";
        case OpCode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OpCode::JUMP_IF_TRUE: return "JUMP_IF_TRUE";
        case OpCode::JUMP_IF_NULL: return "JUMP_IF_NULL";
        case OpCode::JUMP_BACK: return "JUMP_BACK";
        case OpCode::JUMP_IF_FALSE_OR_POP: return "JUMP_IF_FALSE_OR_POP";
        case OpCode::JUMP_IF_TRUE_OR_POP: return "JUMP_IF_TRUE_OR_POP";

        // Functions
        case OpCode::CALL: return "CALL";
        case OpCode::CALL_NATIVE: return "CALL_NATIVE";
        case OpCode::CALL_FAST: return "CALL_FAST";
        case OpCode::CALL_METHOD: return "CALL_METHOD";
        case OpCode::CALL_STATIC: return "CALL_STATIC";
        case OpCode::TAIL_CALL: return "TAIL_CALL";
        case OpCode::RETURN: return "RETURN";
        case OpCode::RETURN_VALUE: return "RETURN_VALUE";

        // Arrays
        case OpCode::ARRAY_GET: return "ARRAY_GET";
        case OpCode::ARRAY_GET_INT: return "ARRAY_GET_INT";
        case OpCode::ARRAY_SET: return "ARRAY_SET";
        case OpCode::ARRAY_SET_INT: return "ARRAY_SET_INT";
        case OpCode::ARRAY_LENGTH: return "ARRAY_LENGTH";
        case OpCode::ARRAY_SET_FIELD: return "ARRAY_SET_FIELD";
        case OpCode::NEW_ARRAY: return "NEW_ARRAY";
        case OpCode::NEW_ARRAY_MULTI: return "NEW_ARRAY_MULTI";

        // Objects
        case OpCode::NEW_OBJECT: return "NEW_OBJECT";
        case OpCode::GET_FIELD: return "GET_FIELD";
        case OpCode::GET_FIELD_FAST: return "GET_FIELD_FAST";
        case OpCode::SET_FIELD: return "SET_FIELD";
        case OpCode::SET_FIELD_FAST: return "SET_FIELD_FAST";
        case OpCode::GET_STATIC: return "GET_STATIC";
        case OpCode::SET_STATIC: return "SET_STATIC";
        case OpCode::INVOKE: return "INVOKE";
        case OpCode::SUPER_INVOKE: return "SUPER_INVOKE";

        // Type Operations
        case OpCode::INSTANCEOF: return "INSTANCEOF";
        case OpCode::CAST: return "CAST";
        case OpCode::CHECK_TYPE: return "CHECK_TYPE";

        // Exception Handling
        case OpCode::THROW: return "THROW";

        // Misc
        case OpCode::NOP: return "NOP";
        case OpCode::LINE: return "LINE";
        case OpCode::SOURCE_FILE: return "SOURCE_FILE";
        case OpCode::LOOP_START: return "LOOP_START";
        case OpCode::LOOP_END: return "LOOP_END";

        default: return "UNKNOWN(" + std::to_string(static_cast<int>(opcode)) + ")";
        }
    }
} // namespace vm::optimization
