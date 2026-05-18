#include "BytecodeOptimizationResult.hpp"

namespace vm::optimization
{
    void BytecodeOptimizationResult::addPassMetrics(const PassMetrics& metrics)
    {
        totalTransformations += metrics.transformationsApplied;
        totalTime += metrics.executionTime;
        passMetrics.push_back(metrics);
    }

    void BytecodeOptimizationResult::addWarning(const std::string& warning)
    {
        warnings.push_back(warning);
    }
}
