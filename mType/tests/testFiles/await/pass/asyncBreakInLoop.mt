// Test break in async loop

import { Int } from "../../lib/primitives/Int.mt";
import { Bool } from "../../lib/primitives/Bool.mt";

print("=== Async Break in Loop Test ===");

function async shouldBreak(int i): Promise<Bool> {
    print("Checking shouldBreak for: " + i);
    return new Bool(i >= 5);
}

function async processWithBreak(): Promise<Int> {
    int total = 0;

    for (int i = 1; i <= 10; i = i + 1) {
        print("Iteration: " + i);

        Bool shouldStop = await shouldBreak(i);
        if (shouldStop.getValue()) {
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
