// Regression test for MYT-265 (JIT-AWAIT deopt route, rejection sub-path).
// Hot async function exceeds the JIT compilation threshold and on each call
// awaits a pending rejection via delayReject. jit_await observes the pending
// promise, throws OSRDeoptException, the interpreter resumes via
// executeAwait's suspend branch, and the catch_ callback writes
// vm->pendingAwaitRejection which is consumed at VirtualMachineLoop's resume
// site.

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Pending Await Rejection (JIT-warmed) ===");

function async hotReject(): Promise<Int> {
    try {
        await delayReject(1, new Exception("boom"));
        return new Int(0);
    } catch (Exception e) {
        return new Int(1);
    }
}

function async main(): Promise<Int> {
    int caught = 0;
    for (int i = 0; i < 200; i = i + 1) {
        Int v = await hotReject();
        caught = caught + v.getValue();
    }
    print("caught=" + caught);
    print("OK");
    return new Int(caught);
}

main();
