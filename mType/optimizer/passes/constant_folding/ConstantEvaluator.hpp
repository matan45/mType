#pragma once

#include "../../../value/ValueType.hpp"
#include "../../../token/TokenType.hpp"
#include "../../../value/StringPool.hpp"
#include "SafeArithmetic.hpp"
#include <optional>
#include <variant>
#include <string>

// Use types from various namespaces
using value::Value;
using value::ValueType;
using token::TokenType;

namespace optimizer {
namespace passes {
namespace constant_folding {

/**
 * @brief Domain service for evaluating constant expressions safely
 *
 * Provides type-safe evaluation of constant expressions that can be folded
 * at compile-time. Uses SafeArithmetic for overflow/underflow detection.
 * Follows conservative approach - when in doubt, don't fold.
 *
 * Design Principles:
 * - Single Responsibility: Only evaluates pure constant expressions
 * - Type Safety: Validates type compatibility before evaluation
 * - Side-Effect Free: Never evaluates expressions with side effects
 */
class ConstantEvaluator {
public:
    /**
     * @brief Evaluate binary operation on two constant values
     * @param left Left operand value
     * @param right Right operand value
     * @param op Operation type (PLUS, MINUS, MULTIPLY, etc.)
     * @return Result value if foldable, std::nullopt otherwise
     */
    static std::optional<Value> evaluateBinaryOp(
        const Value& left,
        const Value& right,
        TokenType op
    );

    /**
     * @brief Evaluate unary operation on a constant value
     * @param operand Operand value
     * @param op Operation type (MINUS, PLUS, NOT)
     * @return Result value if foldable, std::nullopt otherwise
     */
    static std::optional<Value> evaluateUnaryOp(
        const Value& operand,
        TokenType op
    );

    /**
     * @brief Evaluate comparison operation on two constant values
     * @param left Left operand value
     * @param right Right operand value
     * @param op Comparison type (EQUAL, NOT_EQUAL, LESS, etc.)
     * @return Boolean result if foldable, std::nullopt otherwise
     */
    static std::optional<Value> evaluateComparison(
        const Value& left,
        const Value& right,
        TokenType op
    );

    /**
     * @brief Evaluate logical operation on two boolean values
     * @param left Left boolean value
     * @param right Right boolean value
     * @param op Logical operation (AND, OR)
     * @return Boolean result if foldable, std::nullopt otherwise
     */
    static std::optional<Value> evaluateLogicalOp(
        const Value& left,
        const Value& right,
        TokenType op
    );

    /**
     * @brief Evaluate type cast on a constant value
     * @param value Value to cast
     * @param targetType Target value type
     * @return Casted value if foldable, std::nullopt otherwise
     */
    static std::optional<Value> evaluateTypeCast(
        const Value& value,
        ValueType targetType
    );

    /**
     * @brief Check if a value can be folded (is a constant literal)
     * @param value Value to check
     * @return true if value is a foldable constant (int, float, bool, string, null)
     */
    static bool isFoldableValue(const Value& value);

    /**
     * @brief Check if an operation is foldable (no side effects)
     * @param op Operation type
     * @return true if operation can be safely folded
     */
    static bool isFoldableOperation(TokenType op);

private:
    // ==================== Arithmetic Helpers ====================

    static std::optional<Value> evaluateIntArithmetic(
        int64_t left,
        int64_t right,
        TokenType op
    );

    static std::optional<Value> evaluateFloatArithmetic(
        float left,
        float right,
        TokenType op
    );

    static std::optional<Value> evaluateMixedArithmetic(
        const Value& left,
        const Value& right,
        TokenType op
    );

    // ==================== Comparison Helpers ====================

    static std::optional<Value> compareIntegers(int64_t left, int64_t right, TokenType op);
    static std::optional<Value> compareFloats(float left, float right, TokenType op);
    static std::optional<Value> compareBools(bool left, bool right, TokenType op);
    static std::optional<Value> compareStrings(
        const std::string& left,
        const std::string& right,
        TokenType op
    );
    static std::optional<Value> compareNulls(TokenType op);

    // ==================== String Operations ====================

    static std::optional<Value> evaluateStringConcat(
        const Value& left,
        const Value& right
    );

    static std::string valueToString(const Value& value);

    // ==================== Type Conversion ====================

    static std::optional<Value> convertToInt(const Value& value);
    static std::optional<Value> convertToFloat(const Value& value);
    static std::optional<Value> convertToBool(const Value& value);
    static std::optional<Value> convertToString(const Value& value);

    // ==================== Type Detection ====================

    static bool isIntValue(const Value& value);
    static bool isFloatValue(const Value& value);
    static bool isBoolValue(const Value& value);
    static bool isStringValue(const Value& value);
    static bool isNullValue(const Value& value);
};

} // namespace constant_folding
} // namespace passes
} // namespace optimizer
