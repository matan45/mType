// Test: Private static method access violation from external context
class MathUtils {
    private static function square(int x): int {
        return x * x;
    }

    public static function squareAndAdd(int x, int y):int {
        return square(x) + y;
    }
}

print(MathUtils::square(5));  // ERROR: Cannot access private static method
