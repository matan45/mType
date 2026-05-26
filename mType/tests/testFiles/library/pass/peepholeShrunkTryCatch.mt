// MYT-368 regression fixture. Exercises ExceptionHandler::computeSearchLimit,
// which reads FunctionMetadata.instructionCount to bound the try-block scan.
// Before MYT-368, a stale-larger count let the search walk past the
// function's true end into adjacent bytecode. The peephole-foldable
// arithmetic prologue is the trigger that gets a few instructions removed
// from f()'s body.

import * from "../../lib/exceptions/Exception.mt";

function f(): int {
    int a = 1 + 2;
    int b = 3 * 4;
    int c = a + b;
    try {
        throw new Exception("boom");
    } catch (Exception e) {
        return -1;
    }
    return c;
}

print(f());
