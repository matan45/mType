// MathLib: Custom error classes for cross-library try-catch testing

import * from "../../../../lib/exceptions/Exception.mt";

class MathException extends Exception {
    public constructor(string message) : super(message) {
    }
}

class DivisionByZeroException extends MathException {
    public constructor() : super("Division by zero") {
    }
}

class SafeMath {
    public static function divide(int a, int b): int {
        if (b == 0) {
            throw new DivisionByZeroException();
        }
        return a / b;
    }

    public static function sqrt(int x): float {
        if (x < 0) {
            throw new MathException("Cannot compute square root of negative number");
        }
        // Simple approximation
        float result = 1.0;
        for (int i = 0; i < 10; i = i + 1) {
            result = (result + x / result) / 2.0;
        }
        return result;
    }
}
