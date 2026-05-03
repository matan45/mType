// Regression test for MYT-265 (JIT-AWAIT deopt route, in-loop savedState).
// A hot async function with sequential pending awaits inside its own
// for-loop is called 200 times. Once JIT-compiled, every iteration's await
// goes through jit_await -> OSRDeoptException -> interpreter resume into
// executeAwait's suspend branch, exercising the savedState->stack write
// across multiple resume cycles within a single async function call.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Pending Await In Loop (JIT-warmed) ===");

function async hotLoopAwait(int n): Promise<Int> {
    int s = 0;
    for (int i = 1; i <= n; i = i + 1) {
        await delay(1);
        s = s + i;
    }
    return new Int(s);
}

function async main(): Promise<Int> {
    int total = 0;
    for (int i = 0; i < 200; i = i + 1) {
        Int v = await hotLoopAwait(3);
        total = total + v.getValue();
    }
    print("total=" + total);
    print("OK");
    return new Int(total);
}

main();
