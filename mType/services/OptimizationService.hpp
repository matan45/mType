#pragma once
#include <memory>
#include <string>
#include "../ast/ASTNode.hpp"
#include "../environment/Environment.hpp"

namespace optimizer
{
    class Optimizer;
}

namespace services
{
    /**
     * Service for applying AST optimizations
     * Handles optimization execution and reporting
     */
    class OptimizationService
    {
    private:
        std::unique_ptr<optimizer::Optimizer> optimizer;

    public:
        OptimizationService();
        ~OptimizationService();

        // Apply optimizations to AST
        std::unique_ptr<ast::ASTNode> applyOptimizations(
            std::unique_ptr<ast::ASTNode> ast,
            std::shared_ptr<environment::Environment> environment);
    };
}
