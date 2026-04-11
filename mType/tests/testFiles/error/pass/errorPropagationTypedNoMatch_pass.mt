// MYT-38: Lock in that an exception thrown inside a try whose catch clauses
// don't match its type continues to propagate up to an outer handler — instead
// of being silently swallowed by the inner block. Every existing exception test
// either catches everything or uses a base-class catch-all, so this case is
// not exercised today.
import * from "../../lib/exceptions/Exception.mt";

class SpecificException extends Exception {
    public constructor(string msg): super(msg) {
    }
}

class OtherException extends Exception {
    public constructor(string msg): super(msg) {
    }
}

function thrower(): void {
    throw new OtherException("typedPropagation");
}

function main(): void {
    print("start");
    try {
        try {
            thrower();
            print("inner: should not reach");
        } catch (SpecificException e) {
            print("inner: wrong handler ran (BUG)");
        }
        print("outer: should not reach");
    } catch (OtherException e) {
        print("outer: caught OtherException: " + e.getMessage());
    }
    print("end");
}

main();
