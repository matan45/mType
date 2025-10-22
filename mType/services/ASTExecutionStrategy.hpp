#pragma once
#include "ExecutionStrategy.hpp"
#include <memory>

namespace evaluator
{
    class Evaluator;
}

namespace runtime
{
    class EventLoop;
}

namespace services
{
    /**
     * Execution strategy for AST interpretation mode
     * Executes AST directly using the Evaluator with EventLoop support
     */
    class ASTExecutionStrategy : public ExecutionStrategy
    {
    private:
        evaluator::Evaluator* evaluator;

        // Helper for background EventLoop execution
        value::Value executeWithEventLoop(ast::ASTNode* ast, runtime::EventLoop* eventLoop);

    public:
        explicit ASTExecutionStrategy(evaluator::Evaluator* eval);
        ~ASTExecutionStrategy() override = default;

        value::Value execute(ast::ASTNode* ast) override;
    };
}
