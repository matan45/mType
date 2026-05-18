// JIT-warmed twin of asyncPendingDelayInsideLoopBody. Outer loop is
// 200 iterations to exceed the JIT threshold; inner loop is short so
// the suite stays fast. Pins nested-loop suspend/resume under JIT.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Pending Delay Inside Loop Body (JIT-warmed) ===");

function async hotInner(int seed): Promise<Int> {
    int acc = 0;
    for (int j = 1; j <= 2; j = j + 1) {
        await delay(1);
        acc = acc + (seed * j);
    }
    return new Int(acc);
}

function async main(): Promise<Int> {
    int total = 0;
    for (int i = 1; i <= 200; i = i + 1) {
        Int v = await hotInner(i);
        total = total + v.getValue();
    }
    print("total=" + total);
    print("OK");
    return new Int(total);
}

main();
