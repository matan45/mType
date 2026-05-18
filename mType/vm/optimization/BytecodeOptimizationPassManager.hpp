#pragma once

#include <memory>
#include <string>
#include <vector>
#include "BytecodeOptimizationConfig.hpp"
#include "BytecodeOptimizationResult.hpp"
#include "base/BytecodePass.hpp"
#include "base/BytecodeOptimizationContext.hpp"
#include "../bytecode/BytecodeProgram.hpp"

namespace vm::optimization
{
    /**
     * Runs the registered bytecode passes against a BytecodeProgram in order.
     * Unlike optimizer::OptimizationPassManager, passes run once (no fixed-
     * point iteration) — every pass mutates the program in place and the
     * downstream contract is "passes are safe to run in registration order
     * exactly once," matching the historic BytecodeCompiler post-AST block.
     *
     * Failures (PeepholeOptimizer style) are caught and logged into the
     * result's warnings; the remaining passes still run.
     */
    class BytecodeOptimizationPassManager
    {
    private:
        std::vector<std::unique_ptr<base::BytecodePass>> passes;
        BytecodeOptimizationConfig config;
        BytecodeOptimizationResult lastResult;

    public:
        explicit BytecodeOptimizationPassManager(const BytecodeOptimizationConfig& cfg);
        ~BytecodeOptimizationPassManager() = default;

        void registerPass(std::unique_ptr<base::BytecodePass> pass);
        void registerDefaultPasses();

        BytecodeOptimizationResult runPasses(bytecode::BytecodeProgram& program);

        void setConfig(const BytecodeOptimizationConfig& cfg) { config = cfg; }
        const BytecodeOptimizationConfig& getConfig() const { return config; }

        void enablePass(const std::string& passName);
        void disablePass(const std::string& passName);
        base::BytecodePass* getPass(const std::string& passName);
        std::vector<std::string> getRegisteredPasses() const;

        const BytecodeOptimizationResult& getLastResult() const { return lastResult; }
    };
}
