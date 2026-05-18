// Two static methods with identical parameter signatures - rejected at class registration
class MathUtils {
    public static function sum(int a, int b): int {
        return a + b;
    }

    // ERROR: Duplicate static method signature: 'sum(int, int)'
    public static function sum(int x, int y): int {
        return x + y + 1;
    }
}

function main(): void {
    print(MathUtils.sum(1, 2));
}
