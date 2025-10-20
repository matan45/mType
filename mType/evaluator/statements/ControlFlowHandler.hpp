#pragma once

#include "../base/EvaluationContext.hpp"
#include "../managers/ControlFlowManager.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../interfaces/IStatementEvaluator.hpp"
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
     * - Dependency Inversion: Depends on interfaces
     */
    class ControlFlowHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        ControlFlowManager* flowManager;
        interfaces::IExpressionEvaluator* exprEvaluator;
        interfaces::IStatementEvaluator* stmtEvaluator;

    public:
        explicit ControlFlowHandler(std::shared_ptr<EvaluationContext> ctx,
                                   ControlFlowManager* flowMgr)
            : context(ctx), flowManager(flowMgr), exprEvaluator(nullptr), stmtEvaluator(nullptr) {}

        void setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setStatementEvaluator(interfaces::IStatementEvaluator* evaluator) {
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
