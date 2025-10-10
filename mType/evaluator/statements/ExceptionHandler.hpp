#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/UserException.hpp"
#include <memory>

// Forward declarations
namespace ast {
namespace nodes {
namespace statements {
    class TryNode;
    class CatchNode;
    class ThrowNode;
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
    using namespace value;
    using namespace ast::nodes::statements;

    /**
     * @brief Handles exception handling statements (try/catch/throw/finally)
     *
     * Responsibilities:
     * - Try-catch-finally statement evaluation
     * - Exception matching and catch block selection
     * - Finally block execution (guaranteed)
     * - Throw statement processing
     *
     * Design Principles:
     * - Single Responsibility: Only exception handling
     * - Uses C++ exceptions internally to propagate mType exceptions
     * - Delegates expression evaluation to ExpressionEvaluator
     * - Delegates statement execution to StatementEvaluator
     */
    class ExceptionHandler {
    private:
        std::shared_ptr<EvaluationContext> context;
        evaluator::ExpressionEvaluator* exprEvaluator;
        evaluator::StatementEvaluator* stmtEvaluator;

    public:
        explicit ExceptionHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr), stmtEvaluator(nullptr) {}

        void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        void setStatementEvaluator(evaluator::StatementEvaluator* evaluator) {
            stmtEvaluator = evaluator;
        }

        /**
         * @brief Evaluate a try-catch-finally statement
         *
         * Executes try block, catches exceptions, matches with catch blocks,
         * and ensures finally block execution
         */
        Value evaluateTry(TryNode* node);

        /**
         * @brief Evaluate a throw statement
         *
         * Throws a UserException with the evaluated expression value
         */
        Value evaluateThrow(ThrowNode* node);

    private:
        /**
         * @brief Find matching catch block for an exception
         *
         * @param exception The exception to match
         * @param catchBlocks Available catch blocks
         * @return Pointer to matching catch block, or nullptr if no match
         */
        CatchNode* findMatchingCatch(const errors::UserException& exception,
                                     const std::vector<std::unique_ptr<CatchNode>>& catchBlocks);

        /**
         * @brief Execute finally block if present
         *
         * @param finallyBlock The finally block to execute (can be nullptr)
         */
        void executeFinallyBlock(ast::ASTNode* finallyBlock);

        /**
         * @brief Get exception type name from a value
         *
         * @param exceptionValue The exception value
         * @return Type name string
         */
        std::string getExceptionTypeName(const Value& exceptionValue);
    };

} // namespace statements
} // namespace evaluator
