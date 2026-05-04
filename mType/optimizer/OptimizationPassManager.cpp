#include "OptimizationPassManager.hpp"
#include "passes/ConstantFoldingPass.hpp"
#include "passes/DeadCodeEliminationPass.hpp"
#include "passes/EscapeAnalysisPass.hpp"
#include "passes/StructuralEqualitySynthesisPass.hpp"

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
        // Register passes based on optimization config
        // ORDER: StructuralEqualitySynthesis -> Constant Folding -> Dead Code Elimination
        //
        // Rationale:
        // 1. StructuralEqualitySynthesis (MYT-274) runs FIRST so synthesized
        //    hashCode/equals bodies flow through downstream passes (constant
        //    folding may simplify their internal arithmetic, dead-code may
        //    drop unreachable branches in a synthesized null-guard).
        // 2. Constant Folding exposes unreachable code (e.g., if(false) branches)
        // 3. Dead Code Elimination removes unreachable code after control flow terminators
        //
        // These will run in fixed-point iteration until no changes occur

        if (config.isStructuralEqualitySynthesisEnabled())
        {
            registerPass(std::make_unique<passes::StructuralEqualitySynthesisPass>());
        }

        if (config.isConstantFoldingEnabled())
        {
            registerPass(std::make_unique<passes::ConstantFoldingPass>());
        }

        if (config.isDeadCodeEliminationEnabled())
        {
            registerPass(std::make_unique<passes::DeadCodeEliminationPass>());
        }

        // MYT-134: Escape analysis runs after constant folding + dead-code
        // elimination. The earlier passes can remove branches that would
        // otherwise have made locals appear to escape (e.g., unreachable
        // `return local;` in a dead branch).
        if (config.isEscapeAnalysisEnabled())
        {
            registerPass(std::make_unique<passes::EscapeAnalysisPass>());
        }
    }

    std::unique_ptr<ast::ASTNode> OptimizationPassManager::runPasses(
        std::unique_ptr<ast::ASTNode> ast,
        base::OptimizationContext& context)
    {
        lastResult = OptimizationResult();

        // Fixed-point iteration: run passes until no modifications occur
        size_t iteration = 0;
        const size_t maxIterations = config.getMaxPassIterations();
        bool anyPassModified = false;

        do
        {
            anyPassModified = false;
            iteration++;

            // Track metrics for this iteration (for verbose logging)
            std::vector<PassMetrics> iterationMetrics;

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

                // Reset context modification flag for this pass
                context.setModified(false);

                // Run the pass
                try
                {
                    ast = pass->optimize(std::move(ast), context);

                    // Report metrics to both the overall result and iteration tracking
                    OptimizationResult iterationResult;
                    pass->reportMetrics(iterationResult);
                    pass->reportMetrics(lastResult);

                    // Store metrics for this iteration's logging
                    if (!iterationResult.getPassMetrics().empty())
                    {
                        iterationMetrics.push_back(iterationResult.getPassMetrics()[0]);
                    }

                    // Track if this pass modified the AST
                    if (context.wasModified())
                    {
                        anyPassModified = true;
                    }
                }
                catch (const std::exception& e)
                {
                    lastResult.addError(
                        "Pass '" + pass->getName() + "' failed: " + e.what()
                    );
                    lastResult.setSuccessful(false);
                    return ast; // Exit on error
                }
            }
        }
        while (anyPassModified && iteration < maxIterations);

        // Warn if we hit max iterations (potential infinite loop)
        if (iteration >= maxIterations && anyPassModified)
        {
            lastResult.addWarning(
                "Optimization reached maximum iteration limit (" +
                std::to_string(maxIterations) +
                "). AST may not be fully optimized."
            );
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
