// Test nested async calls with multiple levels of concurrency

import { Int } from "../../lib/primitives/Int.mt";

print("=== Nested Concurrency Test ===");

class Data {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function async innerAsync1(): Promise<Data> {
    print("Inner async 1");
    Data d = new Data(10);
    return d;
}

function async innerAsync2(): Promise<Data> {
    print("Inner async 2");
    Data d = new Data(20);
    return d;
}

function async middleAsync(): Promise<Data> {
    print("Middle async starting");

    // Create nested concurrent operations
    Promise<Data> p1 = innerAsync1();
    Promise<Data> p2 = innerAsync2();

    Data d1 = await p1;
    print("Inner 1 resolved: " + d1.getValue());

    Data d2 = await p2;
    print("Inner 2 resolved: " + d2.getValue());

    int sum = d1.getValue() + d2.getValue();
    print("Middle sum: " + sum);

    Data result = new Data(sum);
    return result;
}

function async outerAsync(): Promise<Int> {
    print("Outer async starting");

    // Create multiple middle-level operations
    Promise<Data> pm1 = middleAsync();
    Promise<Data> pm2 = middleAsync();

    Data md1 = await pm1;
    print("Middle 1 complete: " + md1.getValue());

    Data md2 = await pm2;
    print("Middle 2 complete: " + md2.getValue());

    int total = md1.getValue() + md2.getValue();
    print("Outer total: " + total);

    return new Int(total);
}

function async main(): Promise<Int> {
    Int result = await outerAsync();
    print("Final result: " + result);
    return result;
}

main();
