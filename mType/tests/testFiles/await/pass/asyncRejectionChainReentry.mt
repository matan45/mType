// Regression test for MYT-114.
// Exercises checkCompletedPromises re-entry: chained async frames where each
// outer level catches a rejection propagated from the inner level. The
// rejection bubbles through checkCompletedPromises once per suspended caller
// within a single tick; before the fix, any .catch handler touching the loop
// could re-lock queueMutex on the same thread and deadlock.

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Rejection Chain Reentry ===");

function async innermost(): Promise<Int> {
    print("innermost: throwing");
    throw new Exception("inner failure");
    return new Int(0);
}

function async middle(): Promise<Int> {
    print("middle: before await");
    try {
        Int v = await innermost();
        print("middle: should not print");
        return v;
    } catch (Exception e) {
        print("middle: caught " + e.getMessage());
        throw new Exception("middle rethrow");
    }
    return new Int(0);
}

function async outer(): Promise<Int> {
    print("outer: before await");
    try {
        Int v = await middle();
        print("outer: should not print");
        return v;
    } catch (Exception e) {
        print("outer: caught " + e.getMessage());
        return new Int(7);
    }
    return new Int(0);
}

function async main(): Promise<Int> {
    Int result = await outer();
    print("main: result=" + result);
    print("OK");
    return result;
}

main();
