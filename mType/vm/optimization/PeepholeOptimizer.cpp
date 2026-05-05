#include "PeepholeOptimizer.hpp"
#include <cstddef>
#include <cstdint>
#include "patterns/ConstantFoldingPattern.hpp"
#include "patterns/DeadCodePattern.hpp"
#include "patterns/AlgebraicSimplificationPattern.hpp"
#include "patterns/JumpThreadingPattern.hpp"
#include "patterns/StrengthReductionPattern.hpp"
#include "patterns/NullSequencePattern.hpp"
#include "patterns/StackOptimizationPattern.hpp"
#include "patterns/TypeSpecializationPattern.hpp"
#include "patterns/AbstractClassPattern.hpp"
#include "patterns/SuperinstructionPattern.hpp"
#include <algorithm>
#include <chrono>
#include <sstream>

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
        config.timeoutMs = 5000.0;
        return config;
    }

    PeepholeOptimizer::Config PeepholeOptimizer::Config::forDebugMode()
    {
        Config config;
        config.enableAggressiveOptimizations = false;
        config.maxPasses = 3;
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

        // Priority 50: Strength Reduction
        registerPattern(std::make_unique<patterns::StrengthReductionPattern>());

        // Priority 45: Null Sequence Elimination
        registerPattern(std::make_unique<patterns::NullSequencePattern>());

        // Priority 40: Stack Optimization
        registerPattern(std::make_unique<patterns::StackOptimizationPattern>());

        // Priority 30: Type Specialization (minor perf gain)
        registerPattern(std::make_unique<patterns::TypeSpecializationPattern>());

        // Priority 20: Abstract Class Validation (validation pattern)
        registerPattern(std::make_unique<patterns::AbstractClassPattern>());

        // Priority 5: MYT-202 superinstruction fusion — runs after other
        // reductions have converged so we fuse the final, settled instruction
        // sequence. Fusion doesn't expose further reductions, so running last
        // is correct and avoids re-ordering sensitivity.
        registerPattern(std::make_unique<patterns::SuperinstructionPattern>());

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

            // Update exception table offsets
            // This ensures try-catch-finally handler offsets remain correct after instruction removal
            program.updateExceptionTableOffsets(offset + replacement.originalLength, delta);

            // NOTE: CFG/dataflow re-analysis is NOT done here for performance reasons.
            // Most peephole patterns are local and don't require global CFG updates.
            // Re-analysis happens:
            // - After each pattern if config.validateAfterEachPass (runSinglePass:257-258)
            // - At the end of each pass unconditionally (optimize:141-142)
            // This avoids O(K×N) redundant analysis where K = patterns applied per pass
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
                uint32_t target = static_cast<uint32_t>(instr.operands[0]);

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
                        // CRITICAL: Invalid offset detected - fail fast to prevent bytecode corruption
                        throw std::runtime_error(
                            "Peephole optimization error: Jump offset update resulted in invalid target. "
                            "Opcode: " + std::string(getOpCodeName(instr.opcode)) +
                            ", Instruction: " + std::to_string(i) +
                            ", Old target: " + std::to_string(target) +
                            ", New target: " + std::to_string(newTarget) +
                            ", Instruction count: " + std::to_string(program.getInstructionCount()) +
                            ". This indicates a bug in the optimizer.");
                    }
                }
            }

            // CRITICAL: Also update LAMBDA instruction offsets
            // LAMBDA instruction format: operands[0] = lambda start offset
            if (instr.opcode == bytecode::OpCode::LAMBDA && !instr.operands.empty())
            {
                uint32_t lambdaOffset = static_cast<uint32_t>(instr.operands[0]);

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
                        // CRITICAL: Invalid lambda offset detected - fail fast to prevent bytecode corruption
                        throw std::runtime_error(
                            "Peephole optimization error: Lambda offset update resulted in invalid target. "
                            "Instruction: " + std::to_string(i) +
                            ", Old lambda offset: " + std::to_string(lambdaOffset) +
                            ", New lambda offset: " + std::to_string(newOffset) +
                            ", Instruction count: " + std::to_string(program.getInstructionCount()) +
                            ". This indicates a bug in the optimizer.");
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
} // namespace vm::optimization
