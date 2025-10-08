// Test nested async function calls

class Counter {
    int count;

    public constructor(int c) {
        this.count = c;
    }

    public function increment(): int {
        this.count = this.count + 1;
        return this.count;
    }

    public function getCount(): int {
        return this.count;
    }
}

print("=== Nested Async Calls Test ===");

// Level 1: Create counter
function async createCounter(): Promise<Counter> {
    Counter c = new Counter(0);
    return c;
}

// Level 2: Increment counter
function async incrementCounter(): Promise<Counter> {
    Counter c = await createCounter();
    c.increment();
    return c;
}

// Level 3: Use counter
function async useCounter(): Promise<Counter> {
    Counter c = await incrementCounter();
    c.increment();
    return c;
}

// Main function to run test
function async main(): Promise<Counter> {
    Counter finalCounter = await useCounter();
    print("Final count: " + finalCounter.getCount());
    return finalCounter;
}

main();
