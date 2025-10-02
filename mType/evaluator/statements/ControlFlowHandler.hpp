#pragma once

#include "../base/EvaluationContext.hpp"
#include "../managers/ControlFlowManager.hpp"
#include "../../value/ValueType.hpp"
#include <memory>

// Forward declarations
namespace ast {
namespace nodes {
namespace statements {
    class IfNode;
    class SwitchNode;
}
}
}

namespace evaluator {
    class ExpressionEvaluator;
    class StatementEvaluator;
}

namespace evaluator {
namespace statements {

    using namespace base;
    using namespace managers;
    using namespace value;
    using namespace ast::nodes::statements;

    /**
     * @brief Handles control flow statements (if/switch)
     *
     * Responsibilities:
     * - If/else statement evaluation
     * - Switch/case statement evaluation
     * - Case matching and default case handling
     *
     * Design Principles:
     * - Single Responsibility: Only control flow statements
     * - Delegates expression evaluation to ExpressionEvaluator
     * - Delegates statement execution to StatementEvaluator
     */
    class ControlFlowHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        ControlFlowManager* flowManager;
        evaluator::ExpressionEvaluator* exprEvaluator;
        evaluator::StatementEvaluator* stmtEvaluator;

    public:
        explicit ControlFlowHandler(std::shared_ptr<EvaluationContext> ctx,
                                   ControlFlowManager* flowMgr)
            : context(ctx), flowManager(flowMgr), exprEvaluator(nullptr), stmtEvaluator(nullptr) {}

        void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setStatementEvaluator(evaluator::StatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        /**
         * Evaluate an if/else statement
         */
        Value evaluateIf(IfNode* node);

        /**
         * Evaluate a switch statement
         */
        Value evaluateSwitch(SwitchNode* node);
    };

} // namespace statements
} // namespace evaluator
