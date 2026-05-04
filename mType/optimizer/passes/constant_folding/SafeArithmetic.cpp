#include "SafeArithmetic.hpp"
#include <cstdint>

namespace optimizer {
namespace passes {
namespace constant_folding {

// ==================== Integer Overflow Detection ====================

bool SafeArithmetic::wouldOverflowAdd(int64_t a, int64_t b) {
    // Positive overflow: a > INT64_MAX - b
    if (b > 0 && a > std::numeric_limits<int64_t>::max() - b) {
        return true;
    }
    // Negative overflow (underflow): a < INT64_MIN - b
    if (b < 0 && a < std::numeric_limits<int64_t>::min() - b) {
        return true;
    }
    return false;
}

bool SafeArithmetic::wouldUnderflowSubtract(int64_t a, int64_t b) {
    // Subtracting positive can underflow: a < INT64_MIN + b
    if (b > 0 && a < std::numeric_limits<int64_t>::min() + b) {
        return true;
    }
    // Subtracting negative can overflow: a > INT64_MAX + b
    if (b < 0 && a > std::numeric_limits<int64_t>::max() + b) {
        return true;
    }
    return false;
}

bool SafeArithmetic::wouldOverflowMultiply(int64_t a, int64_t b) {
    // Special cases: multiplication by 0 or 1 never overflows
    if (a == 0 || b == 0 || a == 1 || b == 1) {
        return false;
    }

    // Handle multiplication by -1 specially
    if (a == -1) {
        return b == std::numeric_limits<int64_t>::min();
    }
    if (b == -1) {
        return a == std::numeric_limits<int64_t>::min();
    }

    // General case: check if |a| > INT64_MAX / |b|
    int64_t absA = (a < 0) ? -a : a;
    int64_t absB = (b < 0) ? -b : b;

    // Handle INT64_MIN edge case (abs(INT64_MIN) overflows)
    if (a == std::numeric_limits<int64_t>::min() || b == std::numeric_limits<int64_t>::min()) {
        return true; // Always overflow except for cases handled above
    }

    if (absA > std::numeric_limits<int64_t>::max() / absB) {
        return true;
    }

    return false;
}

// ==================== Integer Safe Operations ====================

std::optional<int64_t> SafeArithmetic::safeAdd(int64_t a, int64_t b) {
    if (wouldOverflowAdd(a, b)) {
        return std::nullopt;
    }
    return a + b;
}

std::optional<int64_t> SafeArithmetic::safeSubtract(int64_t a, int64_t b) {
    if (wouldUnderflowSubtract(a, b)) {
        return std::nullopt;
    }
    return a - b;
}

std::optional<int64_t> SafeArithmetic::safeMultiply(int64_t a, int64_t b) {
    if (wouldOverflowMultiply(a, b)) {
        return std::nullopt;
    }
    return a * b;
}

std::optional<int64_t> SafeArithmetic::safeDivide(int64_t dividend, int64_t divisor) {
    // Division by zero check
    if (divisor == 0) {
        return std::nullopt;
    }

    // Special case: INT64_MIN / -1 causes overflow
    // (because -INT64_MIN = INT64_MAX + 1, which doesn't fit in int64_t)
    if (dividend == std::numeric_limits<int64_t>::min() && divisor == -1) {
        return std::nullopt;
    }

    return dividend / divisor;
}

std::optional<int64_t> SafeArithmetic::safeModulo(int64_t dividend, int64_t divisor) {
    // Modulo by zero check
    if (divisor == 0) {
        return std::nullopt;
    }

    // Note: C++ modulo behavior with negative numbers is well-defined since C++11
    // (dividend % divisor) has the same sign as dividend
    // No overflow issues with modulo
    return dividend % divisor;
}

std::optional<int64_t> SafeArithmetic::safeNegate(int64_t value) {
    // Special case: -INT64_MIN causes overflow
    // (because -INT64_MIN = INT64_MAX + 1)
    if (value == std::numeric_limits<int64_t>::min()) {
        return std::nullopt;
    }
    return -value;
}

// ==================== Float Validation ====================

bool SafeArithmetic::isFiniteFloat(double value) {
    // std::isfinite returns true if value is neither NaN nor Infinity
    return std::isfinite(value);
}

// ==================== Float Safe Operations (Conservative Mode) ====================

std::optional<double> SafeArithmetic::safeAdd(double a, double b) {
    // Conservative mode: reject NaN or Infinity inputs
    if (!isFiniteFloat(a) || !isFiniteFloat(b)) {
        return std::nullopt;
    }

    double result = a + b;

    // Conservative mode: reject NaN or Infinity results
    if (!isFiniteFloat(result)) {
        return std::nullopt;
    }

    return result;
}

std::optional<double> SafeArithmetic::safeSubtract(double a, double b) {
    if (!isFiniteFloat(a) || !isFiniteFloat(b)) {
        return std::nullopt;
    }

    double result = a - b;

    if (!isFiniteFloat(result)) {
        return std::nullopt;
    }

    return result;
}

std::optional<double> SafeArithmetic::safeMultiply(double a, double b) {
    if (!isFiniteFloat(a) || !isFiniteFloat(b)) {
        return std::nullopt;
    }

    double result = a * b;

    if (!isFiniteFloat(result)) {
        return std::nullopt;
    }

    return result;
}

std::optional<double> SafeArithmetic::safeDivide(double dividend, double divisor) {
    if (!isFiniteFloat(dividend) || !isFiniteFloat(divisor)) {
        return std::nullopt;
    }

    // Check for division by zero (exact zero or near-zero)
    if (divisor == 0.0 || std::abs(divisor) < std::numeric_limits<double>::epsilon()) {
        return std::nullopt;
    }

    double result = dividend / divisor;

    if (!isFiniteFloat(result)) {
        return std::nullopt;
    }

    return result;
}

std::optional<double> SafeArithmetic::safeNegate(double value) {
    if (!isFiniteFloat(value)) {
        return std::nullopt;
    }

    // Negation of finite double is always safe
    return -value;
}

} // namespace constant_folding
} // namespace passes
} // namespace optimizer
