// Test: Private static method access violation from external context
class MathUtils {
    private static int square(int x) {
        return x * x;
    }

    public static int squareAndAdd(int x, int y) {
        return square(x) + y;
    }
}

print(MathUtils.square(5));  // ERROR: Cannot access private static method
