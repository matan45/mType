#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include <memory>

// Forward declarations
namespace ast {
namespace nodes {
namespace expressions {
    class VariableNode;
}
namespace classes {
    class MemberAccessNode;
    class NewNode;
}
namespace statements {
    class AssignmentNode;
}
}
}

namespace evaluator {
    class ExpressionEvaluator;
    class ObjectEvaluator;
    class StatementEvaluator;
}

namespace evaluator {
namespace expressions {

    using namespace base;
    using namespace value;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::statements;

    /**
     * @brief Handles access operations (variables, members, objects, assignment)
     *
     * Responsibilities:
     * - Variable access (local, instance, static, qualified)
     * - Member access (object fields, array length)
     * - Object creation (new expressions)
     * - Assignment expressions
     * - 'this' keyword handling
     *
     * Design Principles:
     * - Single Responsibility: Only access operations
     * - Handles complex variable resolution logic
     * - Supports qualified static access (ClassName::fieldName)
     */
    class AccessHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        evaluator::ExpressionEvaluator* exprEvaluator;
        evaluator::ObjectEvaluator* objEvaluator;
        evaluator::StatementEvaluator* stmtEvaluator;

    public:
        explicit AccessHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr), objEvaluator(nullptr), stmtEvaluator(nullptr) {}

        void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setObjectEvaluator(evaluator::ObjectEvaluator* evaluator) {
            objEvaluator = evaluator;
        }

        void setStatementEvaluator(evaluator::StatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        /**
         * Evaluate variable access (includes 'this', static fields, qualified access)
         */
        Value evaluateVariableAccess(VariableNode* node);

        /**
         * Evaluate member access (object.field, array.length)
         */
        Value evaluateMemberAccess(MemberAccessNode* node);

        /**
         * Evaluate object creation (new ClassName(...))
         */
        Value evaluateObjectCreation(NewNode* node);

        /**
         * Evaluate assignment expression (returns the assigned value)
         */
        Value evaluateAssignment(AssignmentNode* node);
    };

} // namespace expressions
} // namespace evaluator
