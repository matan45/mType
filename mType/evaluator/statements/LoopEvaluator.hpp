#pragma once

#include "../base/EvaluationContext.hpp"
#include "../managers/ControlFlowManager.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../interfaces/IStatementEvaluator.hpp"
#include "../interfaces/IObjectEvaluator.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include <memory>

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
     * - Dependency Inversion: Depends on interfaces
     */
    class LoopEvaluator {
    private:
        std::shared_ptr<EvaluationContext> context;
        ControlFlowManager* flowManager;
        interfaces::IExpressionEvaluator* exprEvaluator;
        interfaces::IStatementEvaluator* stmtEvaluator;
        interfaces::IObjectEvaluator* objEvaluator;

    public:
        explicit LoopEvaluator(std::shared_ptr<EvaluationContext> ctx,
                               ControlFlowManager* flowMgr)
            : context(ctx), flowManager(flowMgr),
              exprEvaluator(nullptr), stmtEvaluator(nullptr), objEvaluator(nullptr) {}

        void setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setStatementEvaluator(interfaces::IStatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        void setObjectEvaluator(interfaces::IObjectEvaluator* evaluator) {
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

    private:
        // Helper methods for evaluateForEach refactoring
        void executeForEachIteration(const Value& element, ForEachNode* node);
        std::shared_ptr<value::NativeArray> extractArrayFromCollection(
            std::shared_ptr<runtimeTypes::klass::ObjectInstance> collection,
            ForEachNode* node);
    };

} // namespace statements
} // namespace evaluator
