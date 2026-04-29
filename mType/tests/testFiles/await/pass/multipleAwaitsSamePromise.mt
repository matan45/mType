// Test: a single Promise<Int> instance is awaited twice and yields the same value both times
import * from "../../lib/primitives/Int.mt";

function async produce(): Promise<Int> {
    return new Int(123);
}

function async runTest(): Promise<Int> {
    Promise<Int> shared = produce();

    // First await on the stored promise
    Int first = await shared;
    print("First await: " + first.getValue());

    // Second await on the same stored promise instance
    Int second = await shared;
    print("Second await: " + second.getValue());

    int total = first.getValue() + second.getValue();
    return new Int(total);
}

function async main(): Promise<void> {
    Int sum = await runTest();
    print("Sum: " + sum.getValue());
}

main();
