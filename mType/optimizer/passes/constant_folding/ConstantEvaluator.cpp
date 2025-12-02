#include "ConstantEvaluator.hpp"
#include <sstream>
#include <iomanip>

using value::Value;
using value::ValueType;
using value::InternedString;
using token::TokenType;

namespace optimizer {
namespace passes {
namespace constant_folding {

// ==================== Type Detection ====================

bool ConstantEvaluator::isIntValue(const Value& value) {
    return std::holds_alternative<int64_t>(value);
}

bool ConstantEvaluator::isFloatValue(const Value& value) {
    return std::holds_alternative<float>(value);
}

bool ConstantEvaluator::isBoolValue(const Value& value) {
    return std::holds_alternative<bool>(value);
}

bool ConstantEvaluator::isStringValue(const Value& value) {
    return std::holds_alternative<std::string>(value) ||
           std::holds_alternative<InternedString>(value);
}

bool ConstantEvaluator::isNullValue(const Value& value) {
    return std::holds_alternative<std::monostate>(value) ||
           std::holds_alternative<nullptr_t>(value);
}

bool ConstantEvaluator::isFoldableValue(const Value& value) {
    // Only primitive constants can be folded
    return isIntValue(value) || isFloatValue(value) || isBoolValue(value) ||
           isStringValue(value) || isNullValue(value);
}

bool ConstantEvaluator::isFoldableOperation(TokenType op) {
    // Arithmetic operations
    if (op == TokenType::PLUS || op == TokenType::MINUS ||
        op == TokenType::MULTIPLY || op == TokenType::DIVIDE ||
        op == TokenType::MODULO) {
        return true;
    }

    // Comparison operations
    if (op == TokenType::EQUALS || op == TokenType::NOT_EQUALS ||
        op == TokenType::LESS || op == TokenType::LESS_EQUALS ||
        op == TokenType::GREATER || op == TokenType::GREATER_EQUALS) {
        return true;
    }

    // Logical operations
    if (op == TokenType::AND || op == TokenType::OR || op == TokenType::NOT) {
        return true;
    }

    return false;
}

// ==================== Integer Arithmetic ====================

std::optional<Value> ConstantEvaluator::evaluateIntArithmetic(
    int64_t left,
    int64_t right,
    TokenType op
) {
    switch (op) {
        case TokenType::PLUS: {
            auto result = SafeArithmetic::safeAdd(left, right);
            return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
        }
        case TokenType::MINUS: {
            auto result = SafeArithmetic::safeSubtract(left, right);
            return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
        }
        case TokenType::MULTIPLY: {
            auto result = SafeArithmetic::safeMultiply(left, right);
            return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
        }
        case TokenType::DIVIDE: {
            auto result = SafeArithmetic::safeDivide(left, right);
            return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
        }
        case TokenType::MODULO: {
            auto result = SafeArithmetic::safeModulo(left, right);
            return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
        }
        default:
            return std::nullopt;
    }
}

// ==================== Float Arithmetic ====================

std::optional<Value> ConstantEvaluator::evaluateFloatArithmetic(
    float left,
    float right,
    TokenType op
) {
    switch (op) {
        case TokenType::PLUS: {
            auto result = SafeArithmetic::safeAdd(left, right);
            return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
        }
        case TokenType::MINUS: {
            auto result = SafeArithmetic::safeSubtract(left, right);
            return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
        }
        case TokenType::MULTIPLY: {
            auto result = SafeArithmetic::safeMultiply(left, right);
            return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
        }
        case TokenType::DIVIDE: {
            auto result = SafeArithmetic::safeDivide(left, right);
            return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
        }
        default:
            return std::nullopt; // Modulo not supported for floats
    }
}

// ==================== Mixed Type Arithmetic ====================

std::optional<Value> ConstantEvaluator::evaluateMixedArithmetic(
    const Value& left,
    const Value& right,
    TokenType op
) {
    // Type promotion: int + float -> float
    float leftFloat = 0.0f;
    float rightFloat = 0.0f;

    if (isIntValue(left)) {
        leftFloat = static_cast<float>(std::get<int64_t>(left));
    } else if (isFloatValue(left)) {
        leftFloat = std::get<float>(left);
    } else {
        return std::nullopt;
    }

    if (isIntValue(right)) {
        rightFloat = static_cast<float>(std::get<int64_t>(right));
    } else if (isFloatValue(right)) {
        rightFloat = std::get<float>(right);
    } else {
        return std::nullopt;
    }

    return evaluateFloatArithmetic(leftFloat, rightFloat, op);
}

// ==================== Comparisons ====================

std::optional<Value> ConstantEvaluator::compareIntegers(int64_t left, int64_t right, TokenType op) {
    switch (op) {
        case TokenType::EQUALS: return left == right;
        case TokenType::NOT_EQUALS: return left != right;
        case TokenType::LESS: return left < right;
        case TokenType::LESS_EQUALS: return left <= right;
        case TokenType::GREATER: return left > right;
        case TokenType::GREATER_EQUALS: return left >= right;
        default: return std::nullopt;
    }
}

std::optional<Value> ConstantEvaluator::compareFloats(float left, float right, TokenType op) {
    // Conservative: don't compare if either is NaN or Infinity
    if (!SafeArithmetic::isFiniteFloat(left) || !SafeArithmetic::isFiniteFloat(right)) {
        return std::nullopt;
    }

    switch (op) {
        case TokenType::EQUALS: return left == right;
        case TokenType::NOT_EQUALS: return left != right;
        case TokenType::LESS: return left < right;
        case TokenType::LESS_EQUALS: return left <= right;
        case TokenType::GREATER: return left > right;
        case TokenType::GREATER_EQUALS: return left >= right;
        default: return std::nullopt;
    }
}

std::optional<Value> ConstantEvaluator::compareBools(bool left, bool right, TokenType op) {
    // Only equality comparisons for booleans
    switch (op) {
        case TokenType::EQUALS: return left == right;
        case TokenType::NOT_EQUALS: return left != right;
        default: return std::nullopt; // No <, >, <=, >= for booleans
    }
}

std::optional<Value> ConstantEvaluator::compareStrings(
    const std::string& left,
    const std::string& right,
    TokenType op
) {
    switch (op) {
        case TokenType::EQUALS: return left == right;
        case TokenType::NOT_EQUALS: return left != right;
        case TokenType::LESS: return left < right; // Lexicographic
        case TokenType::LESS_EQUALS: return left <= right;
        case TokenType::GREATER: return left > right;
        case TokenType::GREATER_EQUALS: return left >= right;
        default: return std::nullopt;
    }
}

std::optional<Value> ConstantEvaluator::compareNulls(TokenType op) {
    // null == null is true, null != null is false
    switch (op) {
        case TokenType::EQUALS: return true;
        case TokenType::NOT_EQUALS: return false;
        default: return std::nullopt; // Can't use <, >, <=, >= with null
    }
}

std::optional<Value> ConstantEvaluator::evaluateComparison(
    const Value& left,
    const Value& right,
    TokenType op
) {
    // Both integers
    if (isIntValue(left) && isIntValue(right)) {
        return compareIntegers(std::get<int64_t>(left), std::get<int64_t>(right), op);
    }

    // Both floats
    if (isFloatValue(left) && isFloatValue(right)) {
        return compareFloats(std::get<float>(left), std::get<float>(right), op);
    }

    // Mixed int/float - promote to float
    if ((isIntValue(left) || isFloatValue(left)) &&
        (isIntValue(right) || isFloatValue(right))) {
        float leftFloat = isIntValue(left) ? static_cast<float>(std::get<int64_t>(left))
                                           : std::get<float>(left);
        float rightFloat = isIntValue(right) ? static_cast<float>(std::get<int64_t>(right))
                                             : std::get<float>(right);
        return compareFloats(leftFloat, rightFloat, op);
    }

    // Both booleans
    if (isBoolValue(left) && isBoolValue(right)) {
        return compareBools(std::get<bool>(left), std::get<bool>(right), op);
    }

    // Both strings
    if (isStringValue(left) && isStringValue(right)) {
        std::string leftStr;
        std::string rightStr;

        if (std::holds_alternative<std::string>(left)) {
            leftStr = std::get<std::string>(left);
        } else {
            leftStr = std::get<InternedString>(left).getString();
        }

        if (std::holds_alternative<std::string>(right)) {
            rightStr = std::get<std::string>(right);
        } else {
            rightStr = std::get<InternedString>(right).getString();
        }

        return compareStrings(leftStr, rightStr, op);
    }

    // Both null
    if (isNullValue(left) && isNullValue(right)) {
        return compareNulls(op);
    }

    // One null comparison (null != non-null)
    if (isNullValue(left) || isNullValue(right)) {
        if (op == TokenType::EQUALS) return false;
        if (op == TokenType::NOT_EQUALS) return true;
    }

    return std::nullopt;
}

// ==================== Logical Operations ====================

std::optional<Value> ConstantEvaluator::evaluateLogicalOp(
    const Value& left,
    const Value& right,
    TokenType op
) {
    if (!isBoolValue(left) || !isBoolValue(right)) {
        return std::nullopt;
    }

    bool leftBool = std::get<bool>(left);
    bool rightBool = std::get<bool>(right);

    switch (op) {
        case TokenType::AND:
            return leftBool && rightBool;
        case TokenType::OR:
            return leftBool || rightBool;
        default:
            return std::nullopt;
    }
}

// ==================== String Operations ====================

std::string ConstantEvaluator::valueToString(const Value& value) {
    if (isIntValue(value)) {
        return std::to_string(std::get<int64_t>(value));
    }
    if (isFloatValue(value)) {
        std::ostringstream oss;
        oss << std::get<float>(value);
        return oss.str();
    }
    if (isBoolValue(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<InternedString>(value)) {
        return std::get<InternedString>(value).getString();
    }
    if (isNullValue(value)) {
        return "null";
    }
    return "";
}

std::optional<Value> ConstantEvaluator::evaluateStringConcat(
    const Value& left,
    const Value& right
) {
    std::string result = valueToString(left) + valueToString(right);

    // Use StringPool for memory efficiency
    return Value(value::StringPool::getInstance().intern(result));
}

// ==================== Binary Operations ====================

std::optional<Value> ConstantEvaluator::evaluateBinaryOp(
    const Value& left,
    const Value& right,
    TokenType op
) {
    // String concatenation (PLUS operator with at least one string)
    if (op == TokenType::PLUS && (isStringValue(left) || isStringValue(right))) {
        return evaluateStringConcat(left, right);
    }

    // Arithmetic operations
    if (op == TokenType::PLUS || op == TokenType::MINUS ||
        op == TokenType::MULTIPLY || op == TokenType::DIVIDE ||
        op == TokenType::MODULO) {

        // Both integers
        if (isIntValue(left) && isIntValue(right)) {
            return evaluateIntArithmetic(std::get<int64_t>(left), std::get<int64_t>(right), op);
        }

        // Both floats or mixed int/float
        if ((isIntValue(left) || isFloatValue(left)) &&
            (isIntValue(right) || isFloatValue(right))) {
            return evaluateMixedArithmetic(left, right, op);
        }

        return std::nullopt;
    }

    // Comparison operations
    if (op == TokenType::EQUALS || op == TokenType::NOT_EQUALS ||
        op == TokenType::LESS || op == TokenType::LESS_EQUALS ||
        op == TokenType::GREATER || op == TokenType::GREATER_EQUALS) {
        return evaluateComparison(left, right, op);
    }

    // Logical operations
    if (op == TokenType::AND || op == TokenType::OR) {
        return evaluateLogicalOp(left, right, op);
    }

    return std::nullopt;
}

// ==================== Unary Operations ====================

std::optional<Value> ConstantEvaluator::evaluateUnaryOp(
    const Value& operand,
    TokenType op
) {
    switch (op) {
        case TokenType::MINUS: {
            if (isIntValue(operand)) {
                auto result = SafeArithmetic::safeNegate(std::get<int64_t>(operand));
                return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
            }
            if (isFloatValue(operand)) {
                auto result = SafeArithmetic::safeNegate(std::get<float>(operand));
                return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
            }
            return std::nullopt;
        }

        case TokenType::PLUS: {
            // Unary plus is a no-op for numbers
            if (isIntValue(operand) || isFloatValue(operand)) {
                return operand;
            }
            return std::nullopt;
        }

        case TokenType::NOT: {
            if (isBoolValue(operand)) {
                return !std::get<bool>(operand);
            }
            return std::nullopt;
        }

        default:
            return std::nullopt;
    }
}

// ==================== Type Conversions ====================

std::optional<Value> ConstantEvaluator::convertToInt(const Value& value) {
    if (isIntValue(value)) {
        return value; // Already int
    }
    if (isFloatValue(value)) {
        return static_cast<int64_t>(std::get<float>(value)); // Truncation
    }
    if (isBoolValue(value)) {
        return std::get<bool>(value) ? static_cast<int64_t>(1) : static_cast<int64_t>(0);
    }
    return std::nullopt; // Can't convert strings/null to int at compile-time
}

std::optional<Value> ConstantEvaluator::convertToFloat(const Value& value) {
    if (isFloatValue(value)) {
        return value; // Already float
    }
    if (isIntValue(value)) {
        return static_cast<float>(std::get<int64_t>(value));
    }
    return std::nullopt;
}

std::optional<Value> ConstantEvaluator::convertToBool(const Value& value) {
    if (isBoolValue(value)) {
        return value; // Already bool
    }
    if (isIntValue(value)) {
        return std::get<int64_t>(value) != 0;
    }
    return std::nullopt;
}

std::optional<Value> ConstantEvaluator::convertToString(const Value& value) {
    std::string result = valueToString(value);
    return Value(value::StringPool::getInstance().intern(result));
}

std::optional<Value> ConstantEvaluator::evaluateTypeCast(
    const Value& value,
    ValueType targetType
) {
    switch (targetType) {
        case ValueType::INT:
            return convertToInt(value);
        case ValueType::FLOAT:
            return convertToFloat(value);
        case ValueType::BOOL:
            return convertToBool(value);
        case ValueType::STRING:
            return convertToString(value);
        default:
            return std::nullopt; // Can't fold casts to non-primitive types
    }
}

} // namespace constant_folding
} // namespace passes
} // namespace optimizer
