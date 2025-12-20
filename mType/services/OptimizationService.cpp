#include "OptimizationService.hpp"
#include "../optimizer/Optimizer.hpp"
#include "../optimizer/OptimizationConfig.hpp"

namespace services
{
    OptimizationService::OptimizationService()
    {
        optimizer = std::make_unique<optimizer::Optimizer>(
            optimizer::OptimizationConfig::forRelease()
        );
    }

    OptimizationService::~OptimizationService() = default;

    std::unique_ptr<ast::ASTNode> OptimizationService::applyOptimizations(
        std::unique_ptr<ast::ASTNode> ast,
        std::shared_ptr<environment::Environment> environment)
    {
        if (!optimizer)
        {
            return ast;
        }

        return optimizer->optimize(std::move(ast), environment);
    }
}
