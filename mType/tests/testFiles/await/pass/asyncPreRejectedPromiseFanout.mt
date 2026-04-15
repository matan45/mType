// Regression test for MYT-115.
// Fans out three pre-rejected promises into local variables BEFORE awaiting
// any of them, then awaits each in an independent try/catch. Before the fix
// the first call's synchronous throw bubbled out of the assignment; after the
// fix each call returns a rejected promise and each await surfaces the
// original exception.

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Pre-Rejected Promise Fanout ===");

function async willReject(int tag): Promise<Int> {
    throw new Exception("reject-" + tag);
    return new Int(tag);
}

function async main(): Promise<Int> {
    Promise<Int> p1 = willReject(1);
    Promise<Int> p2 = willReject(2);
    Promise<Int> p3 = willReject(3);

    int caughtCount = 0;

    try {
        Int a = await p1;
        print("unreachable 1");
    } catch (Exception e) {
        print("main: caught " + e.getMessage());
        caughtCount = caughtCount + 1;
    }

    try {
        Int b = await p2;
        print("unreachable 2");
    } catch (Exception e) {
        print("main: caught " + e.getMessage());
        caughtCount = caughtCount + 1;
    }

    try {
        Int c = await p3;
        print("unreachable 3");
    } catch (Exception e) {
        print("main: caught " + e.getMessage());
        caughtCount = caughtCount + 1;
    }

    print("main: caughtCount=" + caughtCount);
    print("OK");
    return new Int(caughtCount);
}

main();
