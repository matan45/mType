// Code that guards a throw site against null. The null path does not throw;
// the non-null path throws and is caught.
import * from "../../lib/exceptions/Exception.mt";

function maybeThrow(Exception? e): void {
    if (e != null) {
        throw e;
    }
    print("guard: null, not throwing");
}

function tryPath(Exception? e, string label): void {
    print("---- " + label + " ----");
    try {
        maybeThrow(e);
        print("after maybeThrow: no exception observed");
    } catch (Exception caught) {
        print("caught: " + caught.getMessage());
    }
}

function main(): void {
    print("Testing null-guarded throw site");

    // Null path: should not throw.
    tryPath(null, "null path");

    // Non-null path: should throw and be caught.
    Exception ex = new Exception("real error");
    tryPath(ex, "non-null path");

    print("Null-checked throw test completed");
}

main();
