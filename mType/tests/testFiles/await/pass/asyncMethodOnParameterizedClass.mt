// Pins async method dispatch on a class parameterized by T where the
// method body awaits delay(1) before returning T. Combines generic
// class type parameter with pending-promise suspend/resume.

import { Int } from "../../lib/primitives/Int.mt";

class Holder<T> {
    public T item;

    public constructor(T value) {
        this.item = value;
    }

    public function async unwrap(): Promise<T> {
        await delay(1);
        return this.item;
    }
}

print("=== Async Method On Parameterized Class ===");

function async main(): Promise<Int> {
    Holder<Int> h = new Holder<Int>(new Int(123));
    Int value = await h.unwrap();
    print("value=" + value.getValue());
    print("OK");
    return value;
}

main();
