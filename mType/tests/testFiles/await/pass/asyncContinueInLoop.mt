// Test continue in async loop

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Continue in Loop Test ===");

function async shouldSkip(int i): Promise<bool> {
    print("Checking shouldSkip for: " + i);
    // Skip even numbers
    int remainder = i % 2;
    return remainder == 0;
}

function async processWithContinue(): Promise<Int> {
    int total = 0;

    for (int i = 1; i <= 8; i = i + 1) {
        print("Iteration: " + i);

        bool skip = await shouldSkip(i);
        if (skip) {
            print("Skipping: " + i);
            continue;
        }

        total = total + i;
        print("Added " + i + ", total now: " + total);
    }

    print("Final total (odd numbers only): " + total);
    return new Int(total);
}

function async main(): Promise<Int> {
    Int result = await processWithContinue();
    print("Result: " + result);
    return result;
}

main();
