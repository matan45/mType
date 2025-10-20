#pragma once

#include "../base/EvaluationContext.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../interfaces/IObjectEvaluator.hpp"
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
     * - Dependency Inversion: Depends on interfaces
     */
    class UnaryOperationHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        interfaces::IExpressionEvaluator* exprEvaluator;
        interfaces::IObjectEvaluator* objEvaluator;

    public:
        explicit UnaryOperationHandler(std::shared_ptr<EvaluationContext> ctx);

        void setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator);

        void setObjectEvaluator(interfaces::IObjectEvaluator* evaluator);

        Value evaluateUnaryOperation(UnaryExpNode* node);
    };

} 
}
