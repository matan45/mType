// Test multiple promises created and awaited in sequence
// Validates that promises execute in correct order

import { Int } from "../../lib/primitives/Int.mt";

print("=== Parallel Promises Test ===");

class Counter {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function async createPromise1(): Promise<Counter> {
    print("Creating promise 1");
    Counter c = new Counter(100);
    return c;
}

function async createPromise2(): Promise<Counter> {
    print("Creating promise 2");
    Counter c = new Counter(200);
    return c;
}

function async createPromise3(): Promise<Counter> {
    print("Creating promise 3");
    Counter c = new Counter(300);
    return c;
}

// Test parallel promise creation then sequential awaiting
function async testParallelCreation(): Promise<Int> {
    print("Starting parallel creation");

    // Create all promises (they start "running")
    Promise<Counter> p1 = createPromise1();
    Promise<Counter> p2 = createPromise2();
    Promise<Counter> p3 = createPromise3();

    print("All promises created");

    // Now await them in sequence
    Counter c1 = await p1;
    print("Promise 1 resolved: " + c1.getValue());

    Counter c2 = await p2;
    print("Promise 2 resolved: " + c2.getValue());

    Counter c3 = await p3;
    print("Promise 3 resolved: " + c3.getValue());

    int total = c1.getValue() + c2.getValue() + c3.getValue();
    print("Total: " + total);

    return new Int(total);
}

function async main(): Promise<Int> {
    Int result = await testParallelCreation();
    print("Final result: " + result);
    return result;
}

main();
