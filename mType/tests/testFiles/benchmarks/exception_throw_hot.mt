// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Companion to try_catch_finally_hot.mt — that one exercises the normal path
// with occasional exceptions; this one drives throw/catch as the dominant path
// (1-in-4 iterations throw), measuring exception-value allocation and
// unwinding cost in a tight loop.

import * from "../lib/exceptions/Exception.mt";

function maybeThrow(int x): int {
    if (x % 4 == 0) {
        throw new Exception("expected");
    }
    return x;
}

function run(int n): int {
    int caught = 0;
    int acc = 0;
    for (int i = 0; i < n; i = i + 1) {
        try {
            acc = acc + maybeThrow(i);
        } catch (Exception e) {
            caught = caught + 1;
        }
    }
    return caught * 1000 + acc;
}

int N = 200000;
int result = run(N);
print("exception_throw_hot result=" + result);
