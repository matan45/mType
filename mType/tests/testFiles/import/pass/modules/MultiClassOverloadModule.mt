public class StringHelper {
    public static function format(string s): string {
        return "[" + s + "]";
    }

    public static function format(string a, string b): string {
        return "[" + a + ", " + b + "]";
    }
}

public class Counter {
    public static function count(int n): int {
        return n + 1;
    }
}
