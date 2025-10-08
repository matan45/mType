// Test async methods in classes

class Result {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

class Calculator {
    int base;

    public constructor(int b) {
        this.base = b;
    }

    // Async instance method
    public function async add(int x): Promise<Result> {
        int sum = this.base + x;
        Result r = new Result(sum);
        return r;
    }

    // Async static method
    public static function async multiply(int a, int b): Promise<Result> {
        int product = a * b;
        Result r = new Result(product);
        return r;
    }
}

print("=== Async Methods Test ===");

// Main function to run test
function async main(): Promise<Result> {
    Calculator calc = new Calculator(10);

    // Test instance async method
    Result r1 = await calc.add(5);
    print("10 + 5 = " + r1.getValue());

    // Test static async method
    Result r2 = await Calculator::multiply(3, 4);
    print("3 * 4 = " + r2.getValue());

    return r1;
}

main();
