// Test break in async loop

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Break in Loop Test ===");

function async shouldBreak(int i): Promise<bool> {
    print("Checking shouldBreak for: " + i);
    return i >= 5;
}

function async processWithBreak(): Promise<Int> {
    int total = 0;

    for (int i = 1; i <= 10; i = i + 1) {
        print("Iteration: " + i);

        bool shouldStop = await shouldBreak(i);
        if (shouldStop) {
            print("Breaking at: " + i);
            break;
        }

        total = total + i;
        print("Added " + i + ", total now: " + total);
    }

    print("Final total: " + total);
    return new Int(total);
}

function async main(): Promise<Int> {
    Int result = await processWithBreak();
    print("Result: " + result);
    return result;
}

main();
