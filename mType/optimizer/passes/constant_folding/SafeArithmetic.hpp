#pragma once

#include <optional>
#include <cstdint>
#include <limits>
#include <cmath>

namespace optimizer {
namespace passes {
namespace constant_folding {

/**
 * @brief Utility class for safe arithmetic operations with overflow/underflow detection
 *
 * Provides static methods to perform arithmetic operations on integers and floats
 * with comprehensive safety checks. Returns std::nullopt when operations would
 * result in undefined behavior (overflow, division by zero, etc.)
 *
 * Design Principle: KISS - Simple, explicit checks over clever optimizations
 */
class SafeArithmetic {
public:
    // ==================== Integer Operations ====================

    /**
     * @brief Safely add two integers with overflow detection
     * @return Result if safe, std::nullopt if overflow would occur
     */
    static std::optional<int64_t> safeAdd(int64_t a, int64_t b);

    /**
     * @brief Safely subtract two integers with underflow detection
     * @return Result if safe, std::nullopt if underflow would occur
     */
    static std::optional<int64_t> safeSubtract(int64_t a, int64_t b);

    /**
     * @brief Safely multiply two integers with overflow detection
     * @return Result if safe, std::nullopt if overflow would occur
     */
    static std::optional<int64_t> safeMultiply(int64_t a, int64_t b);

    /**
     * @brief Safely divide two integers with zero-check
     * @return Result if safe, std::nullopt if divisor is zero
     */
    static std::optional<int64_t> safeDivide(int64_t dividend, int64_t divisor);

    /**
     * @brief Safely compute modulo with zero-check
     * @return Result if safe, std::nullopt if divisor is zero
     */
    static std::optional<int64_t> safeModulo(int64_t dividend, int64_t divisor);

    /**
     * @brief Safely negate an integer with overflow detection
     * @return Result if safe, std::nullopt if negation would overflow (INT_MIN case)
     */
    static std::optional<int64_t> safeNegate(int64_t value);

    // ==================== Float Operations ====================

    /**
     * @brief Safely add two floats (conservative mode - no NaN/Infinity)
     * @return Result if safe, std::nullopt if operands or result are NaN/Infinity
     */
    static std::optional<double> safeAdd(double a, double b);

    /**
     * @brief Safely subtract two floats (conservative mode)
     * @return Result if safe, std::nullopt if operands or result are NaN/Infinity
     */
    static std::optional<double> safeSubtract(double a, double b);

    /**
     * @brief Safely multiply two floats (conservative mode)
     * @return Result if safe, std::nullopt if operands or result are NaN/Infinity
     */
    static std::optional<double> safeMultiply(double a, double b);

    /**
     * @brief Safely divide two floats (conservative mode)
     * @return Result if safe, std::nullopt if division by zero or result is NaN/Infinity
     */
    static std::optional<double> safeDivide(double dividend, double divisor);

    /**
     * @brief Safely negate a float (conservative mode)
     * @return Result if safe, std::nullopt if operand is NaN/Infinity
     */
    static std::optional<double> safeNegate(double value);

    // ==================== Validation Helpers ====================

    /**
     * @brief Check if addition would cause overflow
     */
    static bool wouldOverflowAdd(int64_t a, int64_t b);

    /**
     * @brief Check if subtraction would cause underflow
     */
    static bool wouldUnderflowSubtract(int64_t a, int64_t b);

    /**
     * @brief Check if multiplication would cause overflow
     */
    static bool wouldOverflowMultiply(int64_t a, int64_t b);

    /**
     * @brief Check if float value is finite (not NaN or Infinity)
     */
    static bool isFiniteFloat(double value);
};

} // namespace constant_folding
} // namespace passes
} // namespace optimizer
