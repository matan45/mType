// Test memory stress with many promises

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Memory Stress Test ===");

function async createPromise(int id): Promise<Int> {
    return new Int(id);
}

function async stressTest(int count): Promise<Int> {
    print("Creating " + count + " promises");
    int total = 0;

    for (int i = 1; i <= count; i = i + 1) {
        Int value = await createPromise(i);
        total = total + value.getValue();

        if (i % 25 == 0) {
            print("Progress: " + i + " promises created, sum: " + total);
        }
    }

    print("All promises completed");
    return new Int(total);
}

function async main(): Promise<Int> {
    // Create 100 promises
    Int result = await stressTest(100);
    print("Final sum: " + result);
    return result;
}

main();
