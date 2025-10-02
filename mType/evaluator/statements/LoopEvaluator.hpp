#pragma once

#include "../base/EvaluationContext.hpp"
#include "../managers/ControlFlowManager.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include <memory>

namespace evaluator {
    // Forward declarations
    class ExpressionEvaluator;
    class StatementEvaluator;
    class ObjectEvaluator;
}

namespace evaluator {
namespace statements {

    using namespace base;
    using namespace managers;
    using namespace value;
    using namespace ast::nodes::statements;

    /**
     * @brief Handles loop construct evaluation
     *
     * Responsibilities:
     * - While loop evaluation
     * - Do-while loop evaluation
     * - For loop evaluation
     * - ForEach loop evaluation
     *
     * Design Principles:
     * - Single Responsibility: Only loop constructs
     * - Manages loop entry/exit and break/continue handling
     */
    class LoopEvaluator {
    private:
        std::shared_ptr<EvaluationContext> context;
        ControlFlowManager* flowManager;
        evaluator::ExpressionEvaluator* exprEvaluator;
        evaluator::StatementEvaluator* stmtEvaluator;
        evaluator::ObjectEvaluator* objEvaluator;

    public:
        explicit LoopEvaluator(std::shared_ptr<EvaluationContext> ctx,
                               ControlFlowManager* flowMgr)
            : context(ctx), flowManager(flowMgr),
              exprEvaluator(nullptr), stmtEvaluator(nullptr), objEvaluator(nullptr) {}

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
         * Evaluate while loop
         */
        Value evaluateWhile(WhileNode* node);

        /**
         * Evaluate do-while loop
         */
        Value evaluateDoWhile(DoWhileNode* node);

        /**
         * Evaluate for loop
         */
        Value evaluateFor(ForNode* node);

        /**
         * Evaluate forEach loop
         */
        Value evaluateForEach(ForEachNode* node);
    };

} // namespace statements
} // namespace evaluator
