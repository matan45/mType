#include "OptimizationService.hpp"
#include "../optimizer/Optimizer.hpp"
#include "../optimizer/OptimizationConfig.hpp"
#include <iostream>
#include <iomanip>
#include <string>

namespace services
{
    OptimizationService::OptimizationService(constants::OptimizationLevel level)
        : optimizationLevel(level)
    {
        optimizer = std::make_unique<optimizer::Optimizer>(
            optimizer::OptimizationConfig::forLevel(level)
        );
    }

    OptimizationService::~OptimizationService() = default;

    std::unique_ptr<ast::ASTNode> OptimizationService::applyOptimizations(
        std::unique_ptr<ast::ASTNode> ast,
        std::shared_ptr<environment::Environment> environment,
        bool printReport)
    {
        // Only optimize in Release mode
        if (optimizationLevel != constants::OptimizationLevel::Release || !optimizer)
        {
            return ast;
        }

        if (printReport)
        {
            std::cout << "\n" << std::string(60, '=') << "\n";
            std::cout << "APPLYING AST OPTIMIZATIONS (Release Mode)\n";
            std::cout << std::string(60, '=') << "\n";
        }

        // Count nodes before optimization
        size_t nodesBefore = optimizer->countASTNodes(ast.get());

        if (printReport)
        {
            std::cout << "\nAST Statistics:\n";
            std::cout << "  Total nodes before: " << nodesBefore << "\n";
        }

        // Apply optimizations
        ast = optimizer->optimize(std::move(ast), environment);

        // Count nodes after optimization
        size_t nodesAfter = optimizer->countASTNodes(ast.get());
        size_t nodesRemoved = nodesBefore - nodesAfter;

        if (printReport)
        {
            std::cout << "  Total nodes after:  " << nodesAfter << "\n";
            std::cout << "  Nodes removed:      " << nodesRemoved << "\n";

            if (nodesBefore > 0)
            {
                double reductionPercent = (static_cast<double>(nodesRemoved) / nodesBefore) * 100.0;
                std::cout << "  Reduction:          " << std::fixed << std::setprecision(1)
                         << reductionPercent << "%\n";
            }

            // Get and print optimization results
            auto result = optimizer->getLastResult();
            std::cout << "\n" << result.generateReport() << "\n";
            std::cout << std::string(60, '=') << "\n\n";
        }

        return ast;
    }

    void OptimizationService::setOptimizationLevel(constants::OptimizationLevel level)
    {
        optimizationLevel = level;
        if (optimizer)
        {
            optimizer->setConfig(optimizer::OptimizationConfig::forLevel(level));
        }
    }

    constants::OptimizationLevel OptimizationService::getOptimizationLevel() const
    {
        return optimizationLevel;
    }
}
