class MathUtils {
    public static function max(int a, int b): int {
        if (a > b) {
            return a;
        }
        return b;
    }

    public static function abs(int x): int {
        if (x < 0) {
            return 0 - x;
        }
        return x;
    }
}

print(MathUtils::max(3, 7));
print(MathUtils::abs(-5));
print(MathUtils::abs(10));
