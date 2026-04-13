// MathLib: Utility class with static math functions

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
            return 0 - x;
        }
        return x;
    }

    public static function clamp(int value, int lo, int hi): int {
        if (value < lo) {
            return lo;
        }
        if (value > hi) {
            return hi;
        }
        return value;
    }
}
