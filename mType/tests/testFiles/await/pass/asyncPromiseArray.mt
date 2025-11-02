// Test storing promises in arrays and awaiting them

import { Int } from "../../lib/primitives/Int.mt";

print("=== Promise Array Test ===");

class Result {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function async makePromise(int val): Promise<Result> {
    print("Making promise with value: " + val);
    Result r = new Result(val);
    return r;
}

function async testPromiseArray(): Promise<Int> {
    print("Creating promise array");

    // Create array of promises
    Promise<Result>[] promises = new Promise<Result>[3];
    promises[0] = makePromise(10);
    promises[1] = makePromise(20);
    promises[2] = makePromise(30);

    print("Awaiting promises from array");

    // Await each promise from the array
    Result r1 = await promises[0];
    print("Array[0] resolved: " + r1.getValue());

    Result r2 = await promises[1];
    print("Array[1] resolved: " + r2.getValue());

    Result r3 = await promises[2];
    print("Array[2] resolved: " + r3.getValue());

    int sum = r1.getValue() + r2.getValue() + r3.getValue();
    print("Sum: " + sum);

    return new Int(sum);
}

function async main(): Promise<Int> {
    Int result = await testPromiseArray();
    print("Final: " + result);
    return result;
}

main();
