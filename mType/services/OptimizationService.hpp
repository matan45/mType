#pragma once
#include <memory>
#include <string>
#include "../ast/ASTNode.hpp"
#include "../environment/Environment.hpp"
#include "../constants/ExecutionMode.hpp"

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
        constants::OptimizationLevel optimizationLevel;

    public:
        explicit OptimizationService(constants::OptimizationLevel level);
        ~OptimizationService();

        // Apply optimizations to AST and print report
        std::unique_ptr<ast::ASTNode> applyOptimizations(
            std::unique_ptr<ast::ASTNode> ast,
            std::shared_ptr<environment::Environment> environment,
            bool printReport = true);

        // Change optimization level
        void setOptimizationLevel(constants::OptimizationLevel level);
        constants::OptimizationLevel getOptimizationLevel() const;
    };
}
