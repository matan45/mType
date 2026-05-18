// JIT-warmed twin of asyncSuperChainThreeLevels. Hot top-level
// method invoked 200x; pins three-level super dispatch under
// repeated jit_await OSR deopt back to the interpreter.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Super Chain Three Levels (JIT-warmed) ===");

class A {
    public function async run(): Promise<Int> {
        await delay(1);
        return new Int(1);
    }
}

class B extends A {
    public function async run(): Promise<Int> {
        Int a = await super.run();
        return new Int(a.getValue() + 10);
    }
}

class C extends B {
    public function async run(): Promise<Int> {
        Int b = await super.run();
        return new Int(b.getValue() + 100);
    }
}

function async main(): Promise<Int> {
    C c = new C();
    int total = 0;
    for (int i = 1; i <= 200; i = i + 1) {
        Int r = await c.run();
        total = total + r.getValue();
    }
    print("total=" + total);
    print("OK");
    return new Int(total);
}

main();
