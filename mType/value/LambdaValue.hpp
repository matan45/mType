#pragma once
#include "../evaluator/base/EvaluationContext.hpp"
#include <memory>

namespace value
{
    /**
     * Represents an unevaluated lambda expression
     * that can be converted to interface implementation
     */
    class LambdaValue
    {
    private:
        ast::nodes::expressions::LambdaNode* lambdaNode;
        std::shared_ptr<evaluator::base::EvaluationContext> capturedContext;

    public:
        LambdaValue(ast::nodes::expressions::LambdaNode* node,
                    std::shared_ptr<evaluator::base::EvaluationContext> context)
            : lambdaNode(node), capturedContext(context)
        {
        }

        ast::nodes::expressions::LambdaNode* getLambda() const
        {
            return lambdaNode;
        }

        std::shared_ptr<evaluator::base::EvaluationContext> getContext() const
        {
            return capturedContext;
        }
    };
}
