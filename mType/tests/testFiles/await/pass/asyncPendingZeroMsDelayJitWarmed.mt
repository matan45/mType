// JIT-warmed twin of asyncPendingZeroMsDelay. 200 calls to the hot
// async function force jit_await to observe a pending promise from
// delay(0) and deopt to the interpreter on every iteration; pins that
// the zero-ms path still trips the suspend branch under JIT.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Pending Zero Ms Delay (JIT-warmed) ===");

function async hotZero(int i): Promise<Int> {
    await delay(0);
    return new Int(i);
}

function async main(): Promise<Int> {
    int count = 0;
    for (int i = 1; i <= 200; i = i + 1) {
        Int v = await hotZero(i);
        count = count + 1;
    }
    print("count=" + count);
    print("OK");
    return new Int(count);
}

main();
