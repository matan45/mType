class MathUtils {
    public static function max(int a, int b): int {
        if (a > b) {
            return a;
        }
        return b;
    }

    public static function min(int a, int b): int {
        if (a < b) {
            return a;
        }
        return b;
    }

    public static function abs(int x): int {
        if (x < 0) {
            return -x;
        }
        return x;
    }
}

// Call static methods without creating instance
print(MathUtils::max(10, 5)); // 10
print(MathUtils::min(10, 5)); // 5
print(MathUtils::abs(-7)); // 7
print(MathUtils::abs(7)); // 7