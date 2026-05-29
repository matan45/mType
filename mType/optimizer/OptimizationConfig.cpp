#include "OptimizationConfig.hpp"
#include <cstddef>

namespace optimizer
{
    OptimizationConfig::OptimizationConfig()
        : enableDeadCodeElimination(true)
        , enableConstantFolding(true)
        , enableEscapeAnalysis(true)
        , enableStructuralEqualitySynthesis(true)
        , enableLombokSynthesis(true)
        , maxPassIterations(50)
        , timeoutPerPass(std::chrono::milliseconds(5000))
    {
    }

    OptimizationConfig OptimizationConfig::forRelease()
    {
        return OptimizationConfig();
    }

    OptimizationConfig OptimizationConfig::noOptimization()
    {
        OptimizationConfig config;
        config.enableDeadCodeElimination = false;
        config.enableConstantFolding = false;
        config.enableEscapeAnalysis = false;
        config.enableStructuralEqualitySynthesis = false;
        config.enableLombokSynthesis = false;
        return config;
    }

    OptimizationConfig& OptimizationConfig::setDeadCodeElimination(bool enable)
    {
        enableDeadCodeElimination = enable;
        return *this;
    }

    OptimizationConfig& OptimizationConfig::setConstantFolding(bool enable)
    {
        enableConstantFolding = enable;
        return *this;
    }

    OptimizationConfig& OptimizationConfig::setEscapeAnalysis(bool enable)
    {
        enableEscapeAnalysis = enable;
        return *this;
    }

    OptimizationConfig& OptimizationConfig::setStructuralEqualitySynthesis(bool enable)
    {
        enableStructuralEqualitySynthesis = enable;
        return *this;
    }

    OptimizationConfig& OptimizationConfig::setLombokSynthesis(bool enable)
    {
        enableLombokSynthesis = enable;
        return *this;
    }

    OptimizationConfig& OptimizationConfig::setMaxPassIterations(size_t iterations)
    {
        maxPassIterations = iterations;
        return *this;
    }

    OptimizationConfig& OptimizationConfig::setTimeoutPerPass(std::chrono::milliseconds timeout)
    {
        timeoutPerPass = timeout;
        return *this;
    }

} // namespace optimizer
