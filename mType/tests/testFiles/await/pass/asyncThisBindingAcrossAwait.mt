// Pins that `this` binding survives a real event-loop suspension
// inside an async method. The method reads this.field both before
// and after await delay(1); both reads must observe the same
// instance, and a mutation in between must be visible after resume.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async This Binding Across Await ===");

class Counter {
    public int value;

    public constructor(int v) {
        this.value = v;
    }

    public function async bumpAndRead(): Promise<Int> {
        print("pre: this.value=" + this.value);
        this.value = this.value + 1;
        await delay(1);
        print("post: this.value=" + this.value);
        return new Int(this.value);
    }
}

function async main(): Promise<Int> {
    Counter c = new Counter(10);
    Int after = await c.bumpAndRead();
    print("main: after=" + after.getValue() + " c.value=" + c.value);
    print("OK");
    return after;
}

main();
