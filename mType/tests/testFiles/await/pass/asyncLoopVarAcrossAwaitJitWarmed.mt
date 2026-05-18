// JIT-warmed twin of asyncLoopVarAcrossAwait. The hot async function
// is invoked 200 times; each call sums a captured loop variable
// after suspend/resume. Pins that the iteration variable's storage
// slot survives the OSRDeoptException path on every iteration.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Loop Var Across Await (JIT-warmed) ===");

function async hotLoopVar(int seed): Promise<Int> {
    int snapshot = seed;
    await delay(1);
    int recovered = snapshot;
    return new Int(recovered);
}

function async main(): Promise<Int> {
    int total = 0;
    for (int i = 1; i <= 200; i = i + 1) {
        Int v = await hotLoopVar(i);
        total = total + v.getValue();
    }
    print("total=" + total);
    print("OK");
    return new Int(total);
}

main();
