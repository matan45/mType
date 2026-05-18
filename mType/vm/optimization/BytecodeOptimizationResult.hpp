#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace vm::optimization
{
    struct PassMetrics
    {
        std::string passName;
        size_t transformationsApplied = 0;
        std::chrono::milliseconds executionTime{0};
    };

    /**
     * Per-run summary returned by BytecodeOptimizationService::optimize.
     * Aggregates per-pass metrics and pass-failure messages (PeepholeOptimizer
     * intentionally swallows exceptions and logs to stderr; the failure is
     * also recorded here for test inspection).
     */
    class BytecodeOptimizationResult
    {
    private:
        std::vector<PassMetrics> passMetrics;
        std::vector<std::string> warnings;
        size_t totalTransformations = 0;
        std::chrono::milliseconds totalTime{0};

    public:
        void addPassMetrics(const PassMetrics& metrics);
        void addWarning(const std::string& warning);

        const std::vector<PassMetrics>& getPassMetrics() const { return passMetrics; }
        const std::vector<std::string>& getWarnings() const { return warnings; }
        size_t getTotalTransformations() const { return totalTransformations; }
        std::chrono::milliseconds getTotalTime() const { return totalTime; }
    };
}
