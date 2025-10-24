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
                std::cout << "Pass " << passNum + 1 << ": Applied " << optimizationsThisPass << " optimizations\n";
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

        if (config.verboseOutput)
        {
            std::cout << statistics.toString();
        }

        return anyOptimizationsApplied;
    }

    void PeepholeOptimizer::registerPattern(std::unique_ptr<OptimizationPattern> pattern)
    {
        std::string name = pattern->getName();
        enabledPatterns[name] = true;  // Enable by default
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
                    continue;  // Pattern disabled
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

        // Replace instructions in the program
        program.replaceInstructions(offset, replacement.originalLength, replacement.instructions);

        return true;
    }

    void PeepholeOptimizer::updateJumpOffsets(bytecode::BytecodeProgram& program)
    {
        program.updateAllJumpOffsets();
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
