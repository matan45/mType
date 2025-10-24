#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include "OptimizationPattern.hpp"
#include "analysis/ControlFlowAnalyzer.hpp"
#include "analysis/DataFlowAnalyzer.hpp"
#include "../bytecode/BytecodeProgram.hpp"

namespace vm::optimization
{
    /**
     * Coordinates peephole optimization passes on bytecode programs
     * Orchestrates pattern application, iteration, and statistics tracking
     * Single Responsibility: Coordinate optimization process
     */
    class PeepholeOptimizer
    {
    public:
        /**
         * Optimization statistics
         */
        struct Statistics
        {
            size_t totalPasses = 0;
            size_t instructionsEliminated = 0;
            size_t constantsFolded = 0;
            size_t deadCodeRemoved = 0;
            size_t jumpsOptimized = 0;
            size_t algebraicSimplifications = 0;
            size_t strengthReductions = 0;
            size_t loadStoreOptimizations = 0;
            size_t typeSpecializations = 0;
            size_t stackOptimizations = 0;
            size_t nullSequencesRemoved = 0;
            std::unordered_map<std::string, size_t> patternApplications;
            double totalTimeMs = 0.0;

            void reset();
            std::string toString() const;
        };

        /**
         * Configuration for the optimizer
         */
        struct Config
        {
            bool enableAggressiveOptimizations = true;
            size_t maxPasses = 10;  // Maximum number of optimization passes
            bool verboseOutput = true;
            bool validateAfterEachPass = false;
            double timeoutMs = 5000.0;  // Timeout for all passes

            static Config forReleaseMode();
            static Config forDebugMode();
        };

        /**
         * Constructor
         * @param config Optimization configuration
         */
        explicit PeepholeOptimizer(const Config& config = Config::forReleaseMode());

        /**
         * Optimize a bytecode program
         * @param program The bytecode program to optimize (modified in-place)
         * @return true if any optimizations were applied
         */
        bool optimize(bytecode::BytecodeProgram& program);

        /**
         * Register a custom optimization pattern
         * @param pattern Unique pointer to the pattern
         */
        void registerPattern(std::unique_ptr<OptimizationPattern> pattern);

        /**
         * Enable or disable a specific optimization by name
         * @param name The pattern name
         * @param enabled Whether to enable the pattern
         */
        void enableOptimization(const std::string& name, bool enabled);

        /**
         * Get optimization statistics
         * @return Reference to statistics
         */
        const Statistics& getStatistics() const { return statistics; }

        /**
         * Reset statistics
         */
        void resetStatistics();

        /**
         * Register all default optimization patterns
         */
        void registerDefaultPatterns();

        /**
         * Set verbose logging
         * @param verbose Enable/disable verbose output
         */
        void setVerbose(bool verbose) { config.verboseOutput = verbose; }

    private:
        Config config;
        std::vector<std::unique_ptr<OptimizationPattern>> patterns;
        std::unordered_map<std::string, bool> enabledPatterns;
        analysis::ControlFlowAnalyzer cfgAnalyzer;
        analysis::DataFlowAnalyzer dataFlowAnalyzer;
        Statistics statistics;

        // Logging helpers
        void logOptimizationStart(const bytecode::BytecodeProgram& program);
        void logOptimizationEnd(const bytecode::BytecodeProgram& program);
        void logPatternMatch(const std::string& patternName, size_t offset,
                            const bytecode::BytecodeProgram& program,
                            const OptimizationPattern::Replacement& replacement);
        std::string formatInstruction(const bytecode::BytecodeProgram::Instruction& instr, size_t offset) const;

        /**
         * Run a single optimization pass over the program
         * @param program The bytecode program
         * @return Number of optimizations applied
         */
        size_t runSinglePass(bytecode::BytecodeProgram& program);

        /**
         * Try to apply a pattern at a specific offset
         * @param pattern The optimization pattern
         * @param program The bytecode program
         * @param offset The instruction offset
         * @return true if the pattern was applied
         */
        bool tryApplyPattern(const OptimizationPattern& pattern,
                            bytecode::BytecodeProgram& program,
                            size_t offset);

        /**
         * Update jump offsets after modifications
         * @param program The bytecode program
         */
        void updateJumpOffsets(bytecode::BytecodeProgram& program);

        /**
         * Update jump offsets after a specific replacement
         * @param program The bytecode program
         * @param replacementEndOffset The offset where the replacement ended
         * @param delta The change in instruction count (negative if instructions were removed)
         */
        void updateJumpOffsetsAfterReplacement(bytecode::BytecodeProgram& program, size_t replacementEndOffset, int delta);

        /**
         * Sort patterns by priority
         */
        void sortPatternsByPriority();

        /**
         * Record pattern application in statistics
         */
        void recordPatternApplication(const std::string& patternName);
    };

} // namespace vm::optimization
