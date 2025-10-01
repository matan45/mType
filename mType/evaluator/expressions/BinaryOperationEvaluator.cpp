#include "BinaryOperationEvaluator.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../utils/ValueConverter.hpp"
#include "../../value/StringPool.hpp"
#include "../../errors/MathException.hpp"
#include "../../errors/TypeException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/NativeArray.hpp"
#include <cmath>

namespace evaluator {
namespace expressions {

    Value BinaryOperationEvaluator::evaluateArithmetic(const Value& left, const Value& right, TokenType op)
    {
        // Handle string concatenation for PLUS operator
        if (op == TokenType::PLUS &&
            (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right) ||
             std::holds_alternative<value::InternedString>(left) || std::holds_alternative<value::InternedString>(right)))
        {
            // Use ExpressionEvaluator's toString which calls object's toString() method
            std::string leftStr = exprEvaluator ? exprEvaluator->toString(left) : utils::ValueConverter::toString(left);
            std::string rightStr = exprEvaluator ? exprEvaluator->toString(right) : utils::ValueConverter::toString(right);
            std::string result = leftStr + rightStr;

            auto& pool = value::StringPool::getInstance();
            return pool.intern(result);
        }

        // Handle numeric operations
        bool isFloat = std::holds_alternative<float>(left) || std::holds_alternative<float>(right);

        if (isFloat)
        {
            float leftVal = utils::ValueConverter::toFloat(left);
            float rightVal = utils::ValueConverter::toFloat(right);

            switch (op)
            {
            case TokenType::PLUS: return leftVal + rightVal;
            case TokenType::MINUS: return leftVal - rightVal;
            case TokenType::MULTIPLY: return leftVal * rightVal;
            case TokenType::DIVIDE:
                if (rightVal == 0.0f)
                {
                    throw MathException("Division by zero", SourceLocation{});
                }
                return leftVal / rightVal;
            case TokenType::MODULO:
                if (rightVal == 0.0f)
                {
                    throw MathException("Modulo by zero", SourceLocation{});
                }
                return std::fmod(leftVal, rightVal);
            default:
                throw TypeException("Invalid arithmetic operator", SourceLocation{});
            }
        }
        else
        {
            int leftVal = utils::ValueConverter::toInt(left);
            int rightVal = utils::ValueConverter::toInt(right);

            switch (op)
            {
            case TokenType::PLUS: return leftVal + rightVal;
            case TokenType::MINUS: return leftVal - rightVal;
            case TokenType::MULTIPLY: return leftVal * rightVal;
            case TokenType::DIVIDE:
                if (rightVal == 0)
                {
                    throw MathException("Division by zero", SourceLocation{});
                }
                return leftVal / rightVal;
            case TokenType::MODULO:
                if (rightVal == 0)
                {
                    throw MathException("Modulo by zero", SourceLocation{});
                }
                return leftVal % rightVal;
            default:
                throw TypeException("Invalid arithmetic operator", SourceLocation{});
            }
        }
    }

    Value BinaryOperationEvaluator::evaluateComparison(const Value& left, const Value& right, TokenType op)
    {
        // Handle null comparisons (both std::monostate and nullptr_t)
        if (std::holds_alternative<std::monostate>(left) || std::holds_alternative<std::monostate>(right) ||
            std::holds_alternative<std::nullptr_t>(left) || std::holds_alternative<std::nullptr_t>(right))
        {
            bool leftNull = std::holds_alternative<std::monostate>(left) || std::holds_alternative<std::nullptr_t>(left);
            bool rightNull = std::holds_alternative<std::monostate>(right) || std::holds_alternative<std::nullptr_t>(right);

            switch (op)
            {
            case TokenType::EQUALS: return leftNull == rightNull;
            case TokenType::NOT_EQUALS: return leftNull != rightNull;
            default:
                throw TypeException("Cannot compare null with relational operators", SourceLocation{});
            }
        }

        // Handle string comparisons
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
        {
            std::string leftStr = std::get<std::string>(left);
            std::string rightStr = std::get<std::string>(right);

            switch (op)
            {
            case TokenType::EQUALS: return leftStr == rightStr;
            case TokenType::NOT_EQUALS: return leftStr != rightStr;
            case TokenType::LESS: return leftStr < rightStr;
            case TokenType::LESS_EQUALS: return leftStr <= rightStr;
            case TokenType::GREATER: return leftStr > rightStr;
            case TokenType::GREATER_EQUALS: return leftStr >= rightStr;
            default:
                throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        }

        // Handle InternedString comparisons
        if (std::holds_alternative<value::InternedString>(left) && std::holds_alternative<value::InternedString>(right))
        {
            std::string leftStr = std::get<value::InternedString>(left).getString();
            std::string rightStr = std::get<value::InternedString>(right).getString();

            switch (op)
            {
            case TokenType::EQUALS: return leftStr == rightStr;
            case TokenType::NOT_EQUALS: return leftStr != rightStr;
            case TokenType::LESS: return leftStr < rightStr;
            case TokenType::LESS_EQUALS: return leftStr <= rightStr;
            case TokenType::GREATER: return leftStr > rightStr;
            case TokenType::GREATER_EQUALS: return leftStr >= rightStr;
            default:
                throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        }

        // Handle mixed string and InternedString comparisons
        if ((std::holds_alternative<std::string>(left) && std::holds_alternative<value::InternedString>(right)) ||
            (std::holds_alternative<value::InternedString>(left) && std::holds_alternative<std::string>(right)))
        {
            std::string leftStr = std::holds_alternative<std::string>(left) ?
                std::get<std::string>(left) : std::get<value::InternedString>(left).getString();
            std::string rightStr = std::holds_alternative<std::string>(right) ?
                std::get<std::string>(right) : std::get<value::InternedString>(right).getString();

            switch (op)
            {
            case TokenType::EQUALS: return leftStr == rightStr;
            case TokenType::NOT_EQUALS: return leftStr != rightStr;
            case TokenType::LESS: return leftStr < rightStr;
            case TokenType::LESS_EQUALS: return leftStr <= rightStr;
            case TokenType::GREATER: return leftStr > rightStr;
            case TokenType::GREATER_EQUALS: return leftStr >= rightStr;
            default:
                throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        }

        // Handle boolean comparisons
        if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right))
        {
            bool leftBool = std::get<bool>(left);
            bool rightBool = std::get<bool>(right);

            switch (op)
            {
            case TokenType::EQUALS: return leftBool == rightBool;
            case TokenType::NOT_EQUALS: return leftBool != rightBool;
            default:
                throw TypeException("Cannot compare booleans with relational operators", SourceLocation{});
            }
        }

        // Handle object comparisons (identity-based)
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(left) ||
            std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(right) ||
            std::holds_alternative<std::shared_ptr<value::NativeArray>>(left) ||
            std::holds_alternative<std::shared_ptr<value::NativeArray>>(right))
        {
            switch (op)
            {
            case TokenType::EQUALS:
            case TokenType::NOT_EQUALS:
                {
                    // Use ValueConverter for consistent object comparison
                    bool areEqual = utils::ValueConverter::compareValues(left, right);
                    return (op == TokenType::EQUALS) ? areEqual : !areEqual;
                }
            default:
                throw TypeException("Cannot use relational operators (<, >, <=, >=) with objects", SourceLocation{});
            }
        }

        // Handle numeric comparisons
        bool isFloat = std::holds_alternative<float>(left) || std::holds_alternative<float>(right);

        if (isFloat)
        {
            float leftVal = utils::ValueConverter::toFloat(left);
            float rightVal = utils::ValueConverter::toFloat(right);

            switch (op)
            {
            case TokenType::EQUALS: return leftVal == rightVal;
            case TokenType::NOT_EQUALS: return leftVal != rightVal;
            case TokenType::LESS: return leftVal < rightVal;
            case TokenType::LESS_EQUALS: return leftVal <= rightVal;
            case TokenType::GREATER: return leftVal > rightVal;
            case TokenType::GREATER_EQUALS: return leftVal >= rightVal;
            default:
                throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        }
        else
        {
            int leftVal = utils::ValueConverter::toInt(left);
            int rightVal = utils::ValueConverter::toInt(right);

            switch (op)
            {
            case TokenType::EQUALS: return leftVal == rightVal;
            case TokenType::NOT_EQUALS: return leftVal != rightVal;
            case TokenType::LESS: return leftVal < rightVal;
            case TokenType::LESS_EQUALS: return leftVal <= rightVal;
            case TokenType::GREATER: return leftVal > rightVal;
            case TokenType::GREATER_EQUALS: return leftVal >= rightVal;
            default:
                throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        }
    }

    Value BinaryOperationEvaluator::evaluateLogical(const Value& left, const Value& right, TokenType op)
    {
        bool leftBool = utils::ValueConverter::isTruthy(left);
        bool rightBool = utils::ValueConverter::isTruthy(right);

        switch (op)
        {
        case TokenType::AND: return leftBool && rightBool;
        case TokenType::OR: return leftBool || rightBool;
        default:
            throw TypeException("Invalid logical operator", SourceLocation{});
        }
    }

    Value BinaryOperationEvaluator::evaluateStringOperation(const Value& left, const Value& right, TokenType op)
    {
        if (op == TokenType::PLUS)
        {
            // Use ExpressionEvaluator's toString which calls object's toString() method
            std::string leftStr = exprEvaluator ? exprEvaluator->toString(left) : utils::ValueConverter::toString(left);
            std::string rightStr = exprEvaluator ? exprEvaluator->toString(right) : utils::ValueConverter::toString(right);

            auto& pool = value::StringPool::getInstance();
            return pool.intern(leftStr + rightStr);
        }
        throw TypeException("Invalid string operator", SourceLocation{});
    }

} // namespace expressions
} // namespace evaluator
