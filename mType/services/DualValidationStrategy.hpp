#pragma once
#include "ExecutionStrategy.hpp"
#include "../environment/Environment.hpp"
#include <memory>
#include <utility>

namespace evaluator
{
    class Evaluator;
}

namespace services
{
    class ImportResolver;
    class OptimizationService;

    /**
     * Execution strategy for dual validation mode
     * Executes both AST and bytecode modes and compares results
     */
    class DualValidationStrategy : public ExecutionStrategy
    {
    private:
        evaluator::Evaluator* evaluator;
        std::shared_ptr<environment::Environment> environment;
        ImportResolver* importResolver;
        OptimizationService* optimizationService;

        // Helpers for dual execution
        std::pair<value::Value, bool> tryExecuteAST(ast::ASTNode* ast);
        std::pair<value::Value, bool> tryExecuteBytecode(ast::ASTNode* ast);

    public:
        DualValidationStrategy(evaluator::Evaluator* eval,
                              std::shared_ptr<environment::Environment> env,
                              ImportResolver* resolver,
                              OptimizationService* optService);
        ~DualValidationStrategy() override = default;

        value::Value execute(ast::ASTNode* ast) override;
    };
}
