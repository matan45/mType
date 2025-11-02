// Test concurrent async functions modifying shared state
// Validates state management with multiple async operations

import { Int } from "../../lib/primitives/Int.mt";

print("=== Concurrent Modification Test ===");

class SharedState {
    int counter;

    public constructor() {
        this.counter = 0;
    }

    public function increment(int amount): void {
        this.counter = this.counter + amount;
    }

    public function getCounter(): int {
        return this.counter;
    }
}

function async modifyState1(SharedState state): Promise<Int> {
    print("Modifier 1 starting");
    state.increment(100);
    print("Modifier 1 incremented by 100");
    return new Int(state.getCounter());
}

function async modifyState2(SharedState state): Promise<Int> {
    print("Modifier 2 starting");
    state.increment(200);
    print("Modifier 2 incremented by 200");
    return new Int(state.getCounter());
}

function async modifyState3(SharedState state): Promise<Int> {
    print("Modifier 3 starting");
    state.increment(300);
    print("Modifier 3 incremented by 300");
    return new Int(state.getCounter());
}

function async testConcurrentModification(): Promise<Int> {
    print("Creating shared state");
    SharedState state = new SharedState();
    print("Initial counter: " + state.getCounter());

    // Sequential awaiting ensures predictable modification order
    Int r1 = await modifyState1(state);
    print("After mod1: " + r1);

    Int r2 = await modifyState2(state);
    print("After mod2: " + r2);

    Int r3 = await modifyState3(state);
    print("After mod3: " + r3);

    print("Final counter: " + state.getCounter());
    return new Int(state.getCounter());
}

function async main(): Promise<Int> {
    Int result = await testConcurrentModification();
    print("Result: " + result);
    return result;
}

main();
