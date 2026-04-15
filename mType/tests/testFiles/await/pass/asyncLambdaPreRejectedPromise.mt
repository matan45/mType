// Regression test for MYT-115: async lambda body that throws BEFORE the first
// await must produce a rejected Promise rather than propagate the exception
// synchronously out of the lambda's invocation site.

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

interface AsyncIntFn {
    function async apply(int x): Promise<Int>;
}

print("=== Async Lambda Pre-Rejected Promise ===");

function async main(): Promise<Int> {
    AsyncIntFn rejecting = async x -> {
        throw new Exception("lambda-reject-" + x);
        return new Int(x);
    };

    Promise<Int> p = rejecting.apply(7);

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
