// Regression test for MYT-115.
// An async function body that throws BEFORE the first await must produce a
// rejected Promise rather than propagate the exception out of the call site.
// Storing the promise in a variable and awaiting it later inside try/catch
// must still surface the original exception.

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Pre-Rejected Promise Stored ===");

function async willReject(int tag): Promise<Int> {
    throw new Exception("reject-" + tag);
    return new Int(tag);
}

function async main(): Promise<Int> {
    Promise<Int> p = willReject(1);

    int caught = 0;
    try {
        Int v = await p;
        print("unreachable");
    } catch (Exception e) {
        print("main: caught " + e.getMessage());
        caught = 1;
    }

    print("main: caught=" + caught);
    print("OK");
    return new Int(caught);
}

main();
