#pragma once

#include <memory>
#include <string>
#include "BytecodeOptimizationConfig.hpp"
#include "BytecodeOptimizationResult.hpp"
#include "BytecodeOptimizationPassManager.hpp"
#include "../bytecode/BytecodeProgram.hpp"

namespace vm::optimization
{
    /**
     * Facade over the bytecode pass pipeline. Mirrors optimizer::Optimizer.
     *
     * Usage:
     *   BytecodeOptimizer opt(BytecodeOptimizationConfig::forRelease());
     *   auto result = opt.optimize(program);
     */
    class BytecodeOptimizer
    {
    private:
        std::unique_ptr<BytecodeOptimizationPassManager> passManager;
        BytecodeOptimizationConfig config;

    public:
        explicit BytecodeOptimizer(
            const BytecodeOptimizationConfig& cfg = BytecodeOptimizationConfig::forRelease());
        ~BytecodeOptimizer();

        // Mutates program in place; returns metrics summary.
        BytecodeOptimizationResult optimize(bytecode::BytecodeProgram& program);

        void setConfig(const BytecodeOptimizationConfig& cfg);
        const BytecodeOptimizationConfig& getConfig() const { return config; }

        void enablePass(const std::string& passName);
        void disablePass(const std::string& passName);

        const BytecodeOptimizationResult& getLastResult() const;
    };
}
