// Test private async methods

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Private Method Test ===");

class Calculator {
    int secretValue;

    public constructor(int secret) {
        this.secretValue = secret;
    }

    private function async computeSecret(int input): Promise<Int> {
        print("Computing with secret: " + this.secretValue);
        int result = input + this.secretValue;
        return new Int(result);
    }

    public function async calculate(int value): Promise<Int> {
        print("Public calculate called with: " + value);
        Int result = await this.computeSecret(value);
        print("Calculation result: " + result);
        return result;
    }
}

function async main(): Promise<Int> {
    Calculator calc = new Calculator(42);

    Int r1 = await calc.calculate(10);
    print("Result 1: " + r1);

    Int r2 = await calc.calculate(20);
    print("Result 2: " + r2);

    return r2;
}

main();
