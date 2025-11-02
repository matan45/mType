// Test large promise chain (50+ chained promises)

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Large Promise Chain Test ===");

function async chainStep(int step, int accumulator): Promise<Int> {
    int newValue = accumulator + step;
    return new Int(newValue);
}

function async buildLargeChain(int length): Promise<Int> {
    print("Building chain of length: " + length);
    int result = 0;

    for (int i = 1; i <= length; i = i + 1) {
        Int step = await chainStep(i, result);
        result = step.getValue();

        if (i % 10 == 0) {
            print("Progress: " + i + " steps, value: " + result);
        }
    }

    print("Chain complete, final value: " + result);
    return new Int(result);
}

function async main(): Promise<Int> {
    // Create a chain of 50 promises
    Int result = await buildLargeChain(50);
    print("Final result: " + result);
    return result;
}

main();
