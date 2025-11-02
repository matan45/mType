// Test await in while loop

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Await in While Loop Test ===");

function async fetchNext(int current): Promise<Int> {
    print("Fetching next value for: " + current);
    int next = current + 1;
    return new Int(next);
}

function async whileLoopTest(): Promise<Int> {
    int value = 1;

    while (value < 10) {
        print("Current value: " + value);
        Int nextVal = await fetchNext(value);
        value = nextVal.getValue();
    }

    print("Loop terminated at: " + value);
    return new Int(value);
}

function async main(): Promise<Int> {
    Int result = await whileLoopTest();
    print("Final: " + result);
    return result;
}

main();
