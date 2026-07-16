// Regression test for MYT-265 (JIT-AWAIT deopt route).
// Hot async function exceeds the JIT compilation threshold (100 calls), but
// function-level AWAIT must remain interpreted until the JIT can materialize
// an exact deopt frame state. Restarting the body at the call boundary would
// repeat the side effect before delay(ms). The counter below pins exactly-once
// behavior while the normal interpreter suspend/resume path resolves the wait.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Pending Await Suspend (JIT-warmed) ===");

int sideEffects = 0;

function async hotResolve(int i): Promise<Int> {
    // This effect must happen exactly once per invocation. Function-level
    // JIT deopt used to restart the body after reaching the pending await,
    // repeating every effect that preceded it.
    sideEffects = sideEffects + 1;
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
    print("sideEffects=" + sideEffects);
    print("OK");
    return new Int(sum);
}

main();
