#include "OptimizationPassManager.hpp"
#include "passes/DeadCodeEliminationPass.hpp"
#include "passes/UnusedDeclarationEliminationPass.hpp"
#include <stdexcept>

namespace optimizer
{
    OptimizationPassManager::OptimizationPassManager(const OptimizationConfig& cfg)
        : config(cfg)
    {
    }

    bool OptimizationPassManager::checkDependenciesSatisfied(base::OptimizationPass* pass) const
    {
        // For now, all dependencies are satisfied
        // In the future, we can track which passes have run
        // and check if required passes have completed
        return true;
    }

    void OptimizationPassManager::registerPass(std::unique_ptr<base::OptimizationPass> pass)
    {
        if (pass)
        {
            passes.push_back(std::move(pass));
        }
    }

    void OptimizationPassManager::registerDefaultPasses()
    {
        // Register passes based on optimization level
        // Order matters! Dead code elimination should run first, then unused declaration elimination

        if (config.isDeadCodeEliminationEnabled())
        {
            registerPass(std::make_unique<passes::DeadCodeEliminationPass>());
        }

        if (config.isUnusedDeclarationEliminationEnabled())
        {
            registerPass(std::make_unique<passes::UnusedDeclarationEliminationPass>());
        }

        // Future passes can be registered here:
        // if (config.isConstantFoldingEnabled()) {
        //     registerPass(std::make_unique<passes::ConstantFoldingPass>());
        // }
        // if (config.isUnreachableCodeRemovalEnabled()) {
        //     registerPass(std::make_unique<passes::UnreachableCodePass>());
        // }
    }

    std::unique_ptr<ast::ASTNode> OptimizationPassManager::runPasses(
        std::unique_ptr<ast::ASTNode> ast,
        base::OptimizationContext& context)
    {
        lastResult = OptimizationResult();

        if (config.isVerboseOutputEnabled())
        {
            // TODO: Add verbose logging
        }

        // Run each enabled pass
        for (auto& pass : passes)
        {
            if (!pass->isEnabled())
            {
                continue;
            }

            // Check dependencies
            if (!checkDependenciesSatisfied(pass.get()))
            {
                lastResult.addWarning(
                    "Skipping pass '" + pass->getName() +
                    "' - dependencies not satisfied"
                );
                continue;
            }

            // Run the pass
            try
            {
                ast = pass->optimize(std::move(ast), context);

                // Report metrics
                pass->reportMetrics(lastResult);

                // Optionally validate after each pass
                if (config.shouldValidateAfterEachPass())
                {
                    // TODO: Implement validation
                }
            }
            catch (const std::exception& e)
            {
                lastResult.addError(
                    "Pass '" + pass->getName() + "' failed: " + e.what()
                );
                lastResult.setSuccessful(false);
                break;
            }
        }

        return ast;
    }

    void OptimizationPassManager::setConfig(const OptimizationConfig& cfg)
    {
        config = cfg;
    }

    void OptimizationPassManager::enablePass(const std::string& passName)
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

    void OptimizationPassManager::disablePass(const std::string& passName)
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

    base::OptimizationPass* OptimizationPassManager::getPass(const std::string& passName)
    {
        for (auto& pass : passes)
        {
            if (pass->getName() == passName)
            {
                return pass.get();
            }
        }
        return nullptr;
    }

    std::vector<std::string> OptimizationPassManager::getRegisteredPasses() const
    {
        std::vector<std::string> names;
        names.reserve(passes.size());
        for (const auto& pass : passes)
        {
            names.push_back(pass->getName());
        }
        return names;
    }
} // namespace optimizer
