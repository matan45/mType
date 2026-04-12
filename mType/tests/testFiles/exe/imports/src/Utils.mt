// Utility classes for import test

class MathUtils {
    public static function add(int a, int b): int {
        return a + b;
    }

    public static function multiply(int a, int b): int {
        return a * b;
    }

    public static function max(int a, int b): int {
        if (a > b) {
            return a;
        }
        return b;
    }
}

class StringUtils {
    public static function repeat(string s, int count): string {
        string result = "";
        for (int i = 0; i < count; i = i + 1) {
            result = result + s;
        }
        return result;
    }

    public static function padLeft(string s, int width, string pad): string {
        string result = s;
        while (result.length < width) {
            result = pad + result;
        }
        return result;
    }
}
