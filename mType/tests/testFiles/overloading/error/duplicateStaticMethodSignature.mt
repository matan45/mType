// Two static methods with identical parameter signatures - rejected at class registration
class MathUtils {
    static int sum(int a, int b) {
        return a + b;
    }

    // ERROR: Duplicate static method signature: 'sum(int, int)'
    static int sum(int x, int y) {
        return x + y + 1;
    }
}

@Script
function main() {
    print(MathUtils.sum(1, 2));
}
