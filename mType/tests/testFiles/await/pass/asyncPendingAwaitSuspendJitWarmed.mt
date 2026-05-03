// Regression test for MYT-265 (JIT-AWAIT deopt route).
// Hot async function exceeds the JIT compilation threshold (100 calls), so
// subsequent invocations enter jit_await with a still-PENDING
// AsyncPromiseValue from delay(ms). jit_await throws OSRDeoptException, the
// JIT executor's catch unwinds to interpreter mode, and executeAwait runs
// its suspend branch. Pins the resolve sub-path of the JIT-AWAIT deopt route.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Pending Await Suspend (JIT-warmed) ===");

function async hotResolve(int i): Promise<Int> {
    await delay(1);
    return new Int(i);
}

function async main(): Promise<Int> {
    int sum = 0;
    for (int i = 1; i <= 200; i = i + 1) {
        Int v = await hotResolve(i);
        sum = sum + v.getValue();
    }
    print("sum=" + sum);
    print("OK");
    return new Int(sum);
}

main();
