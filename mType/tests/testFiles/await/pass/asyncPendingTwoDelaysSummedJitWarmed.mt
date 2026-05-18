// JIT-warmed twin of asyncPendingTwoDelaysSummed. The hot async function
// is called 200x (> JIT threshold = 100) so jit_await observes a pending
// AsyncPromiseValue from delay() and throws OSRDeoptException, unwinding
// to the interpreter's suspend branch on every iteration.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Pending Two Delays Summed (JIT-warmed) ===");

function async hotTwoDelays(int i): Promise<Int> {
    await delay(1);
    int mid = i;
    await delay(1);
    return new Int(mid + i);
}

function async main(): Promise<Int> {
    int total = 0;
    for (int i = 1; i <= 200; i = i + 1) {
        Int v = await hotTwoDelays(i);
        total = total + v.getValue();
    }
    print("total=" + total);
    print("OK");
    return new Int(total);
}

main();
