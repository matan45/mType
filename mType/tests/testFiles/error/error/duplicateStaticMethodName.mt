// Error: Duplicate static method name in same class

class MathUtils {
    static function multiply(int a, int b): int {
        return a * b;
    }

    // ERROR: Duplicate static method
    static function multiply(int x, int y): int {
        return x * y * 2;
    }
}

int result = MathUtils.multiply(5, 3);
print("This should not execute");
