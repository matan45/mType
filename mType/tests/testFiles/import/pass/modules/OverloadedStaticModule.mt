public class MathUtils {
    public static function max(int a, int b): int {
        if (a > b) {
            return a;
        }
        return b;
    }

    public static function max(float a, float b): float {
        if (a > b) {
            return a;
        }
        return b;
    }

    public static function max(int a, int b, int c): int {
        int temp = a;
        if (b > temp) {
            temp = b;
        }
        if (c > temp) {
            temp = c;
        }
        return temp;
    }

    public static function abs(int x): int {
        if (x < 0) {
            return -x;
        }
        return x;
    }
}
