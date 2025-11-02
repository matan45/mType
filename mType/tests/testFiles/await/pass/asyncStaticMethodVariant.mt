// Test static async methods variant

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Static Method Variant Test ===");

class MathUtils {
    static int computeCount;

    public static function async add(int a, int b): Promise<Int> {
        print("Static add: " + a + " + " + b);
        MathUtils.computeCount = MathUtils.computeCount + 1;
        int result = a + b;
        return new Int(result);
    }

    public static function async multiply(int a, int b): Promise<Int> {
        print("Static multiply: " + a + " * " + b);
        MathUtils.computeCount = MathUtils.computeCount + 1;
        int result = a * b;
        return new Int(result);
    }

    public static function getComputeCount(): int {
        return MathUtils.computeCount;
    }
}

function async main(): Promise<Int> {
    Int sum = await MathUtils.add(5, 10);
    print("Sum: " + sum);

    Int product = await MathUtils.multiply(5, 10);
    print("Product: " + product);

    Int combined = await MathUtils.add(sum.getValue(), product.getValue());
    print("Combined: " + combined);

    print("Total computations: " + MathUtils.getComputeCount());

    return combined;
}

main();
