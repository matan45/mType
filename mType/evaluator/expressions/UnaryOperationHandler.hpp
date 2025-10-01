#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include <memory>

// Forward declarations
namespace ast {
namespace nodes {
namespace expressions {
    class UnaryExpNode;
}
}
}

namespace evaluator {
    class ExpressionEvaluator;
    class ObjectEvaluator;
}

namespace evaluator {
namespace expressions {

    using namespace base;
    using namespace value;
    using namespace ast::nodes::expressions;

    /**
     * @brief Handles unary operation evaluation
     *
     * Responsibilities:
     * - Unary minus and plus operations
     * - Logical NOT operation
     * - Increment and decrement (prefix and postfix)
     * - Supports both variables and member access
     *
     * Design Principles:
     * - Single Responsibility: Only unary operations
     * - Handles both simple variables and member access expressions
     */
    class UnaryOperationHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        evaluator::ExpressionEvaluator* exprEvaluator;
        evaluator::ObjectEvaluator* objEvaluator;

    public:
        explicit UnaryOperationHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr), objEvaluator(nullptr) {}

        void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setObjectEvaluator(evaluator::ObjectEvaluator* evaluator) {
            objEvaluator = evaluator;
        }

        /**
         * Evaluate unary expression
         */
        Value evaluateUnaryOperation(UnaryExpNode* node);
    };

} // namespace expressions
} // namespace evaluator
