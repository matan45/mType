#include "OptimizationConfig.hpp"

namespace optimizer
{
    OptimizationConfig::OptimizationConfig()
        : enableDeadCodeElimination(true)
        , enableConstantFolding(true)
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
