// Test await in loop condition

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Await in Loop Condition Test ===");

class Counter {
    int count;

    public constructor(int initial) {
        this.count = initial;
    }

    public function getCount(): int {
        return this.count;
    }

    public function decrement(): void {
        this.count = this.count - 1;
    }
}

function async hasMore(Counter c): Promise<bool> {
    int current = c.getCount();
    print("Checking hasMore: " + current);
    return current > 0;
}

function async processLoop(Counter c): Promise<Int> {
    int total = 0;

    while (await hasMore(c)) {
        int val = c.getCount();
        print("Processing: " + val);
        total = total + val;
        c.decrement();
    }

    print("Loop finished, total: " + total);
    return new Int(total);
}

function async main(): Promise<Int> {
    Counter counter = new Counter(5);
    Int result = await processLoop(counter);
    print("Final result: " + result);
    return result;
}

main();
