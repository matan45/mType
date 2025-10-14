// Math library with used and unused classes/methods

public class UsedMath {
    // Used method
    public static function add(int a, int b): int {
        return a + b;
    }

    // UNUSED method - should be removed in Release mode
    public static function subtract(int a, int b): int {
        return a - b;
    }

    // UNUSED method - should be removed in Release mode
    public static function multiply(int a, int b): int {
        return a * b;
    }

    // UNUSED method - should be removed in Release mode
    private static function divide(int a, int b): int {
        if (b == 0) return 0;
        return a / b;
    }
}

// UNUSED class - should be removed in Release mode
public class UnusedMath {
    public static function power(int base, int exp): int {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result = result * base;
        }
        return result;
    }

    public static function factorial(int n): int {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }
}

// UNUSED class - should be removed in Release mode
public class AnotherUnusedClass {
    private int value;

    constructor(int v) {
        value = v;
    }

    public function getValue(): int {
        return value;
    }
}
