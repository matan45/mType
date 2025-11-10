// Test direct async recursion

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Direct Recursion Test ===");

function async countdown(int n): Promise<Int> {
    print("Countdown: " + n);

    if (n <= 0) {
        print("Base case reached");
        return new Int(0);
    }

    Int result = await countdown(n - 1);
    return new Int(result.getValue() + n);
}

function async main(): Promise<Int> {
    Int sum = await countdown(5);
    print("Sum: " + sum);
    return sum;
}

main();
