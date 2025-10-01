#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include <memory>
#include <vector>

// Forward declarations for AST nodes
namespace ast {
namespace nodes {
namespace expressions {
    class LambdaInterfaceInvocationNode;
}
namespace functions {
    class FunctionCallNode;
}
namespace classes {
    class MethodCallNode;
}
}
}

namespace evaluator {
    // Forward declarations
    class ExpressionEvaluator;
    class StatementEvaluator;
    class ObjectEvaluator;
}

namespace evaluator {
namespace expressions {

    using namespace base;
    using namespace value;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::classes;

    /**
     * @brief Handles function and method call evaluation
     *
     * Responsibilities:
     * - Function call evaluation (global and namespaced)
     * - Method call evaluation (instance and static)
     * - Lambda invocation
     * - Argument evaluation and passing
     *
     * Design Principles:
     * - Single Responsibility: Only call operations
     * - Handles generic type resolution for calls
     */
    class CallHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        evaluator::ExpressionEvaluator* exprEvaluator;
        evaluator::StatementEvaluator* stmtEvaluator;
        evaluator::ObjectEvaluator* objEvaluator;

    public:
        explicit CallHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr),
              stmtEvaluator(nullptr), objEvaluator(nullptr) {}

        void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setStatementEvaluator(evaluator::StatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        void setObjectEvaluator(evaluator::ObjectEvaluator* evaluator) {
            objEvaluator = evaluator;
        }

        /**
         * Evaluate function call
         */
        Value evaluateFunctionCall(FunctionCallNode* node);

        /**
         * Evaluate method call
         */
        Value evaluateMethodCall(MethodCallNode* node);

        /**
         * Evaluate lambda interface invocation
         */
        Value evaluateLambdaInterfaceInvocation(LambdaInterfaceInvocationNode* node);
    };

} // namespace expressions
} // namespace evaluator
