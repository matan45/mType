// Regression test for MYT-114.
// Awaits several rejecting async functions sequentially inside try/catch.
// Every rejection flows through checkCompletedPromises; the fix invokes
// resultPromise->reject outside queueMutex so any .catch handler that
// re-enters the loop cannot deadlock on the same thread.

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Parallel Rejection Fanout ===");

function async willReject(int tag): Promise<Int> {
    throw new Exception("reject-" + tag);
    return new Int(tag);
}

function async main(): Promise<Int> {
    int caughtCount = 0;

    try {
        Int a = await willReject(1);
        print("unreachable 1");
    } catch (Exception e) {
        print("main: caught " + e.getMessage());
        caughtCount = caughtCount + 1;
    }

    try {
        Int b = await willReject(2);
        print("unreachable 2");
    } catch (Exception e) {
        print("main: caught " + e.getMessage());
        caughtCount = caughtCount + 1;
    }

    try {
        Int c = await willReject(3);
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
