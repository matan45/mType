// Test await in for loop

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Await in For Loop Test ===");

function async getValue(int i): Promise<Int> {
    print("Getting value for: " + i);
    int squared = i * i;
    return new Int(squared);
}

function async sumSquares(int n): Promise<Int> {
    int total = 0;

    for (int i = 1; i <= n; i = i + 1) {
        Int value = await getValue(i);
        int val = value.getValue();
        print("Got value: " + val);
        total = total + val;
    }

    print("Total sum: " + total);
    return new Int(total);
}

function async main(): Promise<Int> {
    Int result = await sumSquares(5);
    print("Final result: " + result);
    return result;
}

main();
