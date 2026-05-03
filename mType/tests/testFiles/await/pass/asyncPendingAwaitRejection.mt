// Regression test for MYT-265.
// Forces AWAIT to register a catch_ callback and resume through
// pendingAwaitRejection after the event loop rejects the promise via
// delayReject(ms, exception).

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Pending Await Rejection ===");

function async main(): Promise<Int> {
    print("main: before delayed rejection");
    try {
        await delayReject(1, new Exception("delayed boom"));
        print("main: should not print");
        return new Int(0);
    } catch (Exception e) {
        print("main: caught " + e.getMessage());
        print("OK");
        return new Int(7);
    }
}

main();
