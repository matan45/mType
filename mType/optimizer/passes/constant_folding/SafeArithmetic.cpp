#include "SafeArithmetic.hpp"

namespace optimizer {
namespace passes {
namespace constant_folding {

// ==================== Integer Overflow Detection ====================

bool SafeArithmetic::wouldOverflowAdd(int a, int b) {
    // Positive overflow: a > INT_MAX - b
    if (b > 0 && a > std::numeric_limits<int>::max() - b) {
        return true;
    }
    // Negative overflow (underflow): a < INT_MIN - b
    if (b < 0 && a < std::numeric_limits<int>::min() - b) {
        return true;
    }
    return false;
}

bool SafeArithmetic::wouldUnderflowSubtract(int a, int b) {
    // Subtracting positive can underflow: a < INT_MIN + b
    if (b > 0 && a < std::numeric_limits<int>::min() + b) {
        return true;
    }
    // Subtracting negative can overflow: a > INT_MAX + b
    if (b < 0 && a > std::numeric_limits<int>::max() + b) {
        return true;
    }
    return false;
}

bool SafeArithmetic::wouldOverflowMultiply(int a, int b) {
    // Special cases: multiplication by 0 or 1 never overflows
    if (a == 0 || b == 0 || a == 1 || b == 1) {
        return false;
    }

    // Handle multiplication by -1 specially
    if (a == -1) {
        return b == std::numeric_limits<int>::min();
    }
    if (b == -1) {
        return a == std::numeric_limits<int>::min();
    }

    // General case: check if |a| > INT_MAX / |b|
    int absA = (a < 0) ? -a : a;
    int absB = (b < 0) ? -b : b;

    // Handle INT_MIN edge case (abs(INT_MIN) overflows)
    if (a == std::numeric_limits<int>::min() || b == std::numeric_limits<int>::min()) {
        return true; // Always overflow except for cases handled above
    }

    if (absA > std::numeric_limits<int>::max() / absB) {
        return true;
    }

    return false;
}

// ==================== Integer Safe Operations ====================

std::optional<int> SafeArithmetic::safeAdd(int a, int b) {
    if (wouldOverflowAdd(a, b)) {
        return std::nullopt;
    }
    return a + b;
}

std::optional<int> SafeArithmetic::safeSubtract(int a, int b) {
    if (wouldUnderflowSubtract(a, b)) {
        return std::nullopt;
    }
    return a - b;
}

std::optional<int> SafeArithmetic::safeMultiply(int a, int b) {
    if (wouldOverflowMultiply(a, b)) {
        return std::nullopt;
    }
    return a * b;
}

std::optional<int> SafeArithmetic::safeDivide(int dividend, int divisor) {
    // Division by zero check
    if (divisor == 0) {
        return std::nullopt;
    }

    // Special case: INT_MIN / -1 causes overflow
    // (because -INT_MIN = INT_MAX + 1, which doesn't fit in int)
    if (dividend == std::numeric_limits<int>::min() && divisor == -1) {
        return std::nullopt;
    }

    return dividend / divisor;
}

std::optional<int> SafeArithmetic::safeModulo(int dividend, int divisor) {
    // Modulo by zero check
    if (divisor == 0) {
        return std::nullopt;
    }

    // Note: C++ modulo behavior with negative numbers is well-defined since C++11
    // (dividend % divisor) has the same sign as dividend
    // No overflow issues with modulo
    return dividend % divisor;
}

std::optional<int> SafeArithmetic::safeNegate(int value) {
    // Special case: -INT_MIN causes overflow
    // (because -INT_MIN = INT_MAX + 1)
    if (value == std::numeric_limits<int>::min()) {
        return std::nullopt;
    }
    return -value;
}

// ==================== Float Validation ====================

bool SafeArithmetic::isFiniteFloat(float value) {
    // std::isfinite returns true if value is neither NaN nor Infinity
    return std::isfinite(value);
}

// ==================== Float Safe Operations (Conservative Mode) ====================

std::optional<float> SafeArithmetic::safeAdd(float a, float b) {
    // Conservative mode: reject NaN or Infinity inputs
    if (!isFiniteFloat(a) || !isFiniteFloat(b)) {
        return std::nullopt;
    }

    float result = a + b;

    // Conservative mode: reject NaN or Infinity results
    if (!isFiniteFloat(result)) {
        return std::nullopt;
    }

    return result;
}

std::optional<float> SafeArithmetic::safeSubtract(float a, float b) {
    if (!isFiniteFloat(a) || !isFiniteFloat(b)) {
        return std::nullopt;
    }

    float result = a - b;

    if (!isFiniteFloat(result)) {
        return std::nullopt;
    }

    return result;
}

std::optional<float> SafeArithmetic::safeMultiply(float a, float b) {
    if (!isFiniteFloat(a) || !isFiniteFloat(b)) {
        return std::nullopt;
    }

    float result = a * b;

    if (!isFiniteFloat(result)) {
        return std::nullopt;
    }

    return result;
}

std::optional<float> SafeArithmetic::safeDivide(float dividend, float divisor) {
    if (!isFiniteFloat(dividend) || !isFiniteFloat(divisor)) {
        return std::nullopt;
    }

    // Check for division by zero (exact zero or near-zero)
    if (divisor == 0.0f || std::abs(divisor) < std::numeric_limits<float>::epsilon()) {
        return std::nullopt;
    }

    float result = dividend / divisor;

    if (!isFiniteFloat(result)) {
        return std::nullopt;
    }

    return result;
}

std::optional<float> SafeArithmetic::safeNegate(float value) {
    if (!isFiniteFloat(value)) {
        return std::nullopt;
    }

    // Negation of finite float is always safe
    return -value;
}

} // namespace constant_folding
} // namespace passes
} // namespace optimizer
