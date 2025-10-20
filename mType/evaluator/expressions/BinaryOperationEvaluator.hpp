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

            Value evaluateArithmetic(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location);

            Value evaluateComparison(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location);

            Value evaluateLogical(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location);

            Value evaluateStringOperation(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location);

        private:
            // Helper methods for evaluateComparison refactoring
            template<typename T>
            bool performComparisonOp(const T& leftVal, const T& rightVal, TokenType op, const errors::SourceLocation& location);

            std::string extractStringValue(const Value& value, const errors::SourceLocation& location);

            Value compareNulls(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location);

            Value compareStrings(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location);

            Value compareBooleans(bool leftBool, bool rightBool, TokenType op, const errors::SourceLocation& location);

            Value compareObjects(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location);

            Value compareNumeric(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location);
        };

        // Template implementation must be in header
        template<typename T>
        bool BinaryOperationEvaluator::performComparisonOp(const T& leftVal, const T& rightVal, TokenType op, const errors::SourceLocation& location)
        {
            switch (op)
            {
            case TokenType::EQUALS: return leftVal == rightVal;
            case TokenType::NOT_EQUALS: return leftVal != rightVal;
            case TokenType::LESS: return leftVal < rightVal;
            case TokenType::LESS_EQUALS: return leftVal <= rightVal;
            case TokenType::GREATER: return leftVal > rightVal;
            case TokenType::GREATER_EQUALS: return leftVal >= rightVal;
            default:
                throw errors::TypeException("Invalid comparison operator", location);
            }
        }
    }
}
