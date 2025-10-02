#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../token/TokenType.hpp"
#include "../../value/ValueType.hpp"
#include <memory>

namespace evaluator
{
    class ExpressionEvaluator;
}

namespace evaluator
{
    namespace expressions
    {
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

        class BinaryOperationEvaluator
        {
        private:
            std::shared_ptr<EvaluationContext> context;
            ExpressionEvaluator* exprEvaluator;

        public:
            explicit BinaryOperationEvaluator(std::shared_ptr<EvaluationContext> ctx);

            void setExpressionEvaluator(ExpressionEvaluator* evaluator);

            Value evaluateArithmetic(const Value& left, const Value& right, TokenType op);

            Value evaluateComparison(const Value& left, const Value& right, TokenType op);

            Value evaluateLogical(const Value& left, const Value& right, TokenType op);

            Value evaluateStringOperation(const Value& left, const Value& right, TokenType op);
        };
    }
}
