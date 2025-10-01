#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../token/TokenType.hpp"
#include "../../value/ValueType.hpp"
#include <memory>

namespace evaluator {
    // Forward declaration from parent namespace
    class ExpressionEvaluator;
}

namespace evaluator {
namespace expressions {

    using namespace base;
    using namespace value;
    using namespace token;

    /**
     * @brief Evaluates binary operation expressions
     *
     * Responsibilities:
     * - Arithmetic operations (+, -, *, /, %)
     * - Comparison operations (==, !=, <, >, <=, >=)
     * - Logical operations (&&, ||)
     * - String concatenation
     *
     * Design Principles:
     * - Single Responsibility: Only binary operations
     * - Type handling: Supports int, float, string, bool, null, objects
     * - Error handling: Division by zero, type mismatches
     */

    class BinaryOperationEvaluator {
    private:
        std::shared_ptr<EvaluationContext> context;
        evaluator::ExpressionEvaluator* exprEvaluator;

    public:
        explicit BinaryOperationEvaluator(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr) {}

        void setExpressionEvaluator(evaluator::ExpressionEvaluator* evaluator) {
            exprEvaluator = evaluator;
        }

        /**
         * Evaluate arithmetic operations
         * Handles: +, -, *, /, %
         * Also handles string concatenation for + operator
         */
        Value evaluateArithmetic(const Value& left, const Value& right, TokenType op);

        /**
         * Evaluate comparison operations
         * Handles: ==, !=, <, >, <=, >=
         * Supports null, string, bool, numeric, and object comparisons
         */
        Value evaluateComparison(const Value& left, const Value& right, TokenType op);

        /**
         * Evaluate logical operations
         * Handles: &&, ||
         */
        Value evaluateLogical(const Value& left, const Value& right, TokenType op);

        /**
         * Evaluate string operations
         * Handles: + (concatenation)
         */
        Value evaluateStringOperation(const Value& left, const Value& right, TokenType op);
    };

} // namespace expressions
} // namespace evaluator
