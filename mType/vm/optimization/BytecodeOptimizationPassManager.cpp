#include "BytecodeOptimizationPassManager.hpp"
#include "passes/LoopOptimizationPass.hpp"
#include "passes/PeepholeOptimizationPass.hpp"
#include "passes/TrivialSetterInliningPass.hpp"
#include "passes/TrivialGetterInliningPass.hpp"
#include "passes/LocalArrayFusionPass.hpp"

#include <chrono>
#include <exception>

namespace vm::optimization
{
    BytecodeOptimizationPassManager::BytecodeOptimizationPassManager(
        const BytecodeOptimizationConfig& cfg)
        : config(cfg)
    {
    }

    void BytecodeOptimizationPassManager::registerPass(std::unique_ptr<base::BytecodePass> pass)
    {
        if (pass)
        {
            passes.push_back(std::move(pass));
        }
    }

    void BytecodeOptimizationPassManager::registerDefaultPasses()
    {
        // Preserves the historic BytecodeCompiler post-AST order:
        //   LoopOptimization -> Peephole -> TrivialSetterInlining ->
        //   TrivialGetterInlining -> LocalArrayFusion.
        if (config.isLoopOptimizationEnabled())
        {
            registerPass(std::make_unique<passes::LoopOptimizationPass>());
        }
        if (config.isPeepholeEnabled())
        {
            registerPass(std::make_unique<passes::PeepholeOptimizationPass>());
        }
        if (config.isTrivialSetterInliningEnabled())
        {
            registerPass(std::make_unique<passes::TrivialSetterInliningPass>());
        }
        if (config.isTrivialGetterInliningEnabled())
        {
            registerPass(std::make_unique<passes::TrivialGetterInliningPass>());
        }
        if (config.isLocalArrayFusionEnabled())
        {
            registerPass(std::make_unique<passes::LocalArrayFusionPass>());
        }
    }

    BytecodeOptimizationResult BytecodeOptimizationPassManager::runPasses(
        bytecode::BytecodeProgram& program)
    {
        lastResult = BytecodeOptimizationResult{};
        base::BytecodeOptimizationContext context(config);

        for (auto& pass : passes)
        {
            if (!pass->isEnabled()) continue;
            if (!config.isPassEnabled(pass->getName())) continue;

            context.setModified(false);
            pass->reset();

            auto start = std::chrono::steady_clock::now();
            try
            {
                pass->optimize(program, context);
            }
            catch (const std::exception& e)
            {
                lastResult.addWarning(
                    "Pass '" + pass->getName() + "' failed: " + e.what());
                // Continue with remaining passes — preserve the
                // BytecodeCompiler.cpp PeepholeOptimizer behaviour of
                // logging and pressing on rather than aborting compile.
                continue;
            }
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);

            pass->reportMetrics(lastResult);
            (void)elapsed;
        }

        return lastResult;
    }

    void BytecodeOptimizationPassManager::enablePass(const std::string& passName)
    {
        for (auto& pass : passes)
        {
            if (pass->getName() == passName)
            {
                pass->setEnabled(true);
                return;
            }
        }
    }

    void BytecodeOptimizationPassManager::disablePass(const std::string& passName)
    {
        for (auto& pass : passes)
        {
            if (pass->getName() == passName)
            {
                pass->setEnabled(false);
                return;
            }
        }
    }

    base::BytecodePass* BytecodeOptimizationPassManager::getPass(const std::string& passName)
    {
        for (auto& pass : passes)
        {
            if (pass->getName() == passName) return pass.get();
        }
        return nullptr;
    }

    std::vector<std::string> BytecodeOptimizationPassManager::getRegisteredPasses() const
    {
        std::vector<std::string> names;
        names.reserve(passes.size());
        for (const auto& pass : passes)
        {
            names.push_back(pass->getName());
        }
        return names;
    }
}
