// JIT-warmed twin of asyncPendingDelayInTryFinally. 200 calls to a hot
// async function with try/finally + delay(1) inside try; pins that the
// finally block runs exactly once per call even when jit_await
// repeatedly deopts to the interpreter mid-try.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Pending Delay In Try Finally (JIT-warmed) ===");

int finallyCounter = 0;

function async hotTryFinally(int i): Promise<Int> {
    try {
        await delay(1);
        return new Int(i);
    } finally {
        finallyCounter = finallyCounter + 1;
    }
}

function async main(): Promise<Int> {
    int sum = 0;
    for (int i = 1; i <= 200; i = i + 1) {
        Int v = await hotTryFinally(i);
        sum = sum + v.getValue();
    }
    print("finallyCounter=" + finallyCounter);
    print("sum=" + sum);
    print("OK");
    return new Int(sum);
}

main();
