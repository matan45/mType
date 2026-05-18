// Pins async super-chain dispatch through three inheritance levels
// where the base calls delay(1). C.run -> B.run (await super) ->
// A.run (await delay(1)). Each await must suspend/resume correctly
// inside the unwinding super chain.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Super Chain Three Levels ===");

class A {
    public function async run(): Promise<Int> {
        await delay(1);
        print("A.run done");
        return new Int(1);
    }
}

class B extends A {
    public function async run(): Promise<Int> {
        Int a = await super.run();
        print("B.run after super=" + a.getValue());
        return new Int(a.getValue() + 10);
    }
}

class C extends B {
    public function async run(): Promise<Int> {
        Int b = await super.run();
        print("C.run after super=" + b.getValue());
        return new Int(b.getValue() + 100);
    }
}

function async main(): Promise<Int> {
    C c = new C();
    Int r = await c.run();
    print("main: r=" + r.getValue());
    print("OK");
    return r;
}

main();
