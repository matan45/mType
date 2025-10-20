#include "BinaryOperationEvaluator.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../utils/ValueConverter.hpp"
#include "../../value/StringPool.hpp"
#include "../../errors/MathException.hpp"
#include "../../errors/TypeException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/NativeArray.hpp"
#include <cmath>

namespace evaluator
{
    namespace expressions
    {
        BinaryOperationEvaluator::BinaryOperationEvaluator(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr)
        {
        }

        void BinaryOperationEvaluator::setExpressionEvaluator(ExpressionEvaluator* evaluator)
        {
            exprEvaluator = evaluator;
        }

        Value BinaryOperationEvaluator::evaluateArithmetic(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location)
        {
            // Handle string concatenation for PLUS operator
            if (op == TokenType::PLUS &&
                (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right) ||
                    std::holds_alternative<InternedString>(left) || std::holds_alternative<
                        InternedString>(right)))
            {
                // Use ExpressionEvaluator's toString which calls object's toString() method
                std::string leftStr = exprEvaluator
                                          ? exprEvaluator->toString(left)
                                          : ValueConverter::toString(left);
                std::string rightStr = exprEvaluator
                                           ? exprEvaluator->toString(right)
                                           : ValueConverter::toString(right);
                std::string result = leftStr + rightStr;

                auto& pool = StringPool::getInstance();
                return pool.intern(result);
            }

            // Handle numeric operations
            bool isFloat = std::holds_alternative<float>(left) || std::holds_alternative<float>(right);

            if (isFloat)
            {
                float leftVal = ValueConverter::toFloat(left);
                float rightVal = ValueConverter::toFloat(right);

                switch (op)
                {
                case TokenType::PLUS: return leftVal + rightVal;
                case TokenType::MINUS: return leftVal - rightVal;
                case TokenType::MULTIPLY: return leftVal * rightVal;
                case TokenType::DIVIDE:
                    if (rightVal == 0.0f)
                    {
                        throw MathException("Division by zero", location);
                    }
                    return leftVal / rightVal;
                case TokenType::MODULO:
                    if (rightVal == 0.0f)
                    {
                        throw MathException("Modulo by zero", location);
                    }
                    return std::fmod(leftVal, rightVal);
                default:
                    throw TypeException("Invalid arithmetic operator", location);
                }
            }
            else
            {
                int leftVal = ValueConverter::toInt(left);
                int rightVal = ValueConverter::toInt(right);

                switch (op)
                {
                case TokenType::PLUS: return leftVal + rightVal;
                case TokenType::MINUS: return leftVal - rightVal;
                case TokenType::MULTIPLY: return leftVal * rightVal;
                case TokenType::DIVIDE:
                    if (rightVal == 0)
                    {
                        throw MathException("Division by zero", location);
                    }
                    return leftVal / rightVal;
                case TokenType::MODULO:
                    if (rightVal == 0)
                    {
                        throw MathException("Modulo by zero", location);
                    }
                    return leftVal % rightVal;
                default:
                    throw TypeException("Invalid arithmetic operator", location);
                }
            }
        }

        Value BinaryOperationEvaluator::evaluateComparison(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location)
        {
            // Handle null comparisons (both std::monostate and nullptr_t)
            if (std::holds_alternative<std::monostate>(left) || std::holds_alternative<std::monostate>(right) ||
                std::holds_alternative<std::nullptr_t>(left) || std::holds_alternative<std::nullptr_t>(right))
            {
                return compareNulls(left, right, op, location);
            }

            // Handle string comparisons (std::string, InternedString, or mixed)
            bool isLeftString = std::holds_alternative<std::string>(left) ||
                                std::holds_alternative<InternedString>(left);
            bool isRightString = std::holds_alternative<std::string>(right) ||
                                 std::holds_alternative<InternedString>(right);

            if (isLeftString && isRightString)
            {
                return compareStrings(left, right, op, location);
            }

            // Handle boolean comparisons
            if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right))
            {
                return compareBooleans(std::get<bool>(left), std::get<bool>(right), op, location);
            }

            // Handle object comparisons (identity-based)
            if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(left) ||
                std::holds_alternative<std::shared_ptr<ObjectInstance>>(right) ||
                std::holds_alternative<std::shared_ptr<NativeArray>>(left) ||
                std::holds_alternative<std::shared_ptr<NativeArray>>(right))
            {
                return compareObjects(left, right, op, location);
            }

            // Handle numeric comparisons (int and float)
            return compareNumeric(left, right, op, location);
        }

        Value BinaryOperationEvaluator::evaluateLogical(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location)
        {
            bool leftBool = ValueConverter::isTruthy(left);
            bool rightBool = ValueConverter::isTruthy(right);

            switch (op)
            {
            case TokenType::AND: return leftBool && rightBool;
            case TokenType::OR: return leftBool || rightBool;
            default:
                throw TypeException("Invalid logical operator", location);
            }
        }

        Value BinaryOperationEvaluator::evaluateStringOperation(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location)
        {
            if (op == TokenType::PLUS)
            {
                // Use ExpressionEvaluator's toString which calls object's toString() method
                std::string leftStr = exprEvaluator
                                          ? exprEvaluator->toString(left)
                                          : ValueConverter::toString(left);
                std::string rightStr = exprEvaluator
                                           ? exprEvaluator->toString(right)
                                           : ValueConverter::toString(right);

                auto& pool = StringPool::getInstance();
                return pool.intern(leftStr + rightStr);
            }
            throw TypeException("Invalid string operator", location);
        }

        // ========== Helper Methods for evaluateComparison Refactoring ==========

        std::string BinaryOperationEvaluator::extractStringValue(const Value& value, const errors::SourceLocation& location)
        {
            if (std::holds_alternative<std::string>(value))
            {
                return std::get<std::string>(value);
            }
            if (std::holds_alternative<InternedString>(value))
            {
                return std::get<InternedString>(value).getString();
            }
            throw TypeException("Value is not a string type", location);
        }

        Value BinaryOperationEvaluator::compareNulls(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location)
        {
            bool leftNull = std::holds_alternative<std::monostate>(left) ||
                            std::holds_alternative<std::nullptr_t>(left);
            bool rightNull = std::holds_alternative<std::monostate>(right) ||
                             std::holds_alternative<std::nullptr_t>(right);

            switch (op)
            {
            case TokenType::EQUALS: return leftNull == rightNull;
            case TokenType::NOT_EQUALS: return leftNull != rightNull;
            default:
                throw TypeException("Cannot compare null with relational operators", location);
            }
        }

        Value BinaryOperationEvaluator::compareStrings(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location)
        {
            std::string leftStr = extractStringValue(left, location);
            std::string rightStr = extractStringValue(right, location);

            return performComparisonOp(leftStr, rightStr, op, location);
        }

        Value BinaryOperationEvaluator::compareBooleans(bool leftBool, bool rightBool, TokenType op, const errors::SourceLocation& location)
        {
            switch (op)
            {
            case TokenType::EQUALS: return leftBool == rightBool;
            case TokenType::NOT_EQUALS: return leftBool != rightBool;
            default:
                throw TypeException("Cannot compare booleans with relational operators", location);
            }
        }

        Value BinaryOperationEvaluator::compareObjects(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location)
        {
            switch (op)
            {
            case TokenType::EQUALS:
            case TokenType::NOT_EQUALS:
                {
                    // Use ValueConverter for consistent object comparison
                    bool areEqual = ValueConverter::compareValues(left, right);
                    return (op == TokenType::EQUALS) ? areEqual : !areEqual;
                }
            default:
                throw TypeException("Cannot use relational operators (<, >, <=, >=) with objects", location);
            }
        }

        Value BinaryOperationEvaluator::compareNumeric(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location)
        {
            bool isFloat = std::holds_alternative<float>(left) || std::holds_alternative<float>(right);

            if (isFloat)
            {
                float leftVal = ValueConverter::toFloat(left);
                float rightVal = ValueConverter::toFloat(right);
                return performComparisonOp(leftVal, rightVal, op, location);
            }
            else
            {
                int leftVal = ValueConverter::toInt(left);
                int rightVal = ValueConverter::toInt(right);
                return performComparisonOp(leftVal, rightVal, op, location);
            }
        }
    }
}
