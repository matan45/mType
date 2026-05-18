// Pins a generic async function that takes Promise<T> and returns
// Promise<T> by awaiting the input. Exercises type-parameter T
// flowing through both the parameter and return type while the
// body re-awaits inside the async wrapper.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Generic Identity Promise ===");

function async <T> identity(Promise<T> p): Promise<T> {
    T value = await p;
    return value;
}

function async produce(): Promise<Int> {
    return new Int(77);
}

function async main(): Promise<Int> {
    Promise<Int> source = produce();
    Int result = await identity<Int>(source);
    print("result=" + result.getValue());
    print("OK");
    return result;
}

main();
