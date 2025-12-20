#include "OptimizationConfig.hpp"

namespace optimizer
{
    OptimizationConfig::OptimizationConfig()
        : enableDeadCodeElimination(true)
        , enableUnusedDeclarationElimination(false)  // Disabled - can't reliably track generics/imports
        , enableConstantFolding(true)
        , enableUnreachableCodeRemoval(true)
        , maxPassIterations(50)
        , timeoutPerPass(std::chrono::milliseconds(5000))
    {
        // Default: safe optimizations enabled
        // UnusedDeclarationElimination is disabled because it can't reliably track:
        // - Generic type instantiations (e.g., ArrayList<String>)
        // - Functions/interfaces used through imports
        // - Implicit method calls
    }

    OptimizationConfig OptimizationConfig::forRelease()
    {
        return OptimizationConfig();
    }

    OptimizationConfig OptimizationConfig::noOptimization()
    {
        OptimizationConfig config;
        config.enableDeadCodeElimination = false;
        config.enableUnusedDeclarationElimination = false;
        config.enableConstantFolding = false;
        config.enableUnreachableCodeRemoval = false;
        return config;
    }

    OptimizationConfig& OptimizationConfig::setDeadCodeElimination(bool enable)
    {
        enableDeadCodeElimination = enable;
        return *this;
    }

    OptimizationConfig& OptimizationConfig::setUnusedDeclarationElimination(bool enable)
    {
        enableUnusedDeclarationElimination = enable;
        return *this;
    }

    OptimizationConfig& OptimizationConfig::setConstantFolding(bool enable)
    {
        enableConstantFolding = enable;
        return *this;
    }

    OptimizationConfig& OptimizationConfig::setUnreachableCodeRemoval(bool enable)
    {
        enableUnreachableCodeRemoval = enable;
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
