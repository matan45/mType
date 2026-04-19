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
    return value::isInt(value);
}

bool ConstantEvaluator::isFloatValue(const Value& value) {
    return value::isFloat(value);
}

bool ConstantEvaluator::isBoolValue(const Value& value) {
    return value::isBool(value);
}

bool ConstantEvaluator::isStringValue(const Value& value) {
    return value::isString(value) ||
           value::isInternedString(value);
}

bool ConstantEvaluator::isNullValue(const Value& value) {
    return value::isVoid(value) ||
           value::isNullType(value);
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
    double left,
    double right,
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
    double leftFloat = 0.0;
    double rightFloat = 0.0;

    if (isIntValue(left)) {
        leftFloat = static_cast<double>(value::asInt(left));
    } else if (isFloatValue(left)) {
        leftFloat = value::asFloat(left);
    } else {
        return std::nullopt;
    }

    if (isIntValue(right)) {
        rightFloat = static_cast<double>(value::asInt(right));
    } else if (isFloatValue(right)) {
        rightFloat = value::asFloat(right);
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

std::optional<Value> ConstantEvaluator::compareFloats(double left, double right, TokenType op) {
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
        return compareIntegers(value::asInt(left), value::asInt(right), op);
    }

    // Both floats
    if (isFloatValue(left) && isFloatValue(right)) {
        return compareFloats(value::asFloat(left), value::asFloat(right), op);
    }

    // Mixed int/float - promote to float
    if ((isIntValue(left) || isFloatValue(left)) &&
        (isIntValue(right) || isFloatValue(right))) {
        double leftFloat = isIntValue(left) ? static_cast<double>(value::asInt(left))
                                           : value::asFloat(left);
        double rightFloat = isIntValue(right) ? static_cast<double>(value::asInt(right))
                                             : value::asFloat(right);
        return compareFloats(leftFloat, rightFloat, op);
    }

    // Both booleans
    if (isBoolValue(left) && isBoolValue(right)) {
        return compareBools(value::asBool(left), value::asBool(right), op);
    }

    // Both strings
    if (isStringValue(left) && isStringValue(right)) {
        std::string leftStr;
        std::string rightStr;

        if (value::isString(left)) {
            leftStr = value::asString(left);
        } else {
            leftStr = value::asInternedString(left).getString();
        }

        if (value::isString(right)) {
            rightStr = value::asString(right);
        } else {
            rightStr = value::asInternedString(right).getString();
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

    bool leftBool = value::asBool(left);
    bool rightBool = value::asBool(right);

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
        return std::to_string(value::asInt(value));
    }
    if (isFloatValue(value)) {
        std::ostringstream oss;
        oss << value::asFloat(value);
        return oss.str();
    }
    if (isBoolValue(value)) {
        return value::asBool(value) ? "true" : "false";
    }
    if (value::isString(value)) {
        return value::asString(value);
    }
    if (value::isInternedString(value)) {
        return value::asInternedString(value).getString();
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
            return evaluateIntArithmetic(value::asInt(left), value::asInt(right), op);
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
                auto result = SafeArithmetic::safeNegate(value::asInt(operand));
                return result.has_value() ? std::optional<Value>(*result) : std::nullopt;
            }
            if (isFloatValue(operand)) {
                auto result = SafeArithmetic::safeNegate(value::asFloat(operand));
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
                return !value::asBool(operand);
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
        return static_cast<int64_t>(value::asFloat(value)); // Truncation
    }
    if (isBoolValue(value)) {
        return value::asBool(value) ? static_cast<int64_t>(1) : static_cast<int64_t>(0);
    }
    return std::nullopt; // Can't convert strings/null to int at compile-time
}

std::optional<Value> ConstantEvaluator::convertToFloat(const Value& value) {
    if (isFloatValue(value)) {
        return value; // Already float
    }
    if (isIntValue(value)) {
        return static_cast<double>(value::asInt(value));
    }
    return std::nullopt;
}

std::optional<Value> ConstantEvaluator::convertToBool(const Value& value) {
    if (isBoolValue(value)) {
        return value; // Already bool
    }
    if (isIntValue(value)) {
        return value::asInt(value) != 0;
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
