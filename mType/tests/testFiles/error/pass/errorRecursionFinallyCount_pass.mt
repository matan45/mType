// Test: a recursive function with try/finally fires its finally once
// per call frame as the throw unwinds the stack. Counter assertion
// verifies finally ran (depth + 1) times for descend(depth=4).
import * from "../../lib/exceptions/Exception.mt";

class Counter {
    public static int finallyHits = 0;
}

function descend(int depth): void {
    try {
        if (depth == 0) {
            throw new Exception("bottom");
        }
        descend(depth - 1);
    } finally {
        Counter::finallyHits = Counter::finallyHits + 1;
    }
}

function main(): void {
    try {
        descend(4);
    } catch (Exception e) {
        print("caught: " + e.getMessage());
    }
    print("finallyHits=" + Counter::finallyHits);
}
main();
