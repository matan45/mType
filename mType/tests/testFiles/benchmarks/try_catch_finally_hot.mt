// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises try/finally setup on the normal path plus occasional exception
// dispatch through catch blocks.

import * from "../lib/exceptions/Exception.mt";

int finallyCount = 0;

function guarded(int x): int {
    try {
        if (x == -1) {
            throw new Exception("cold path");
        }
        return x + 1;
    } catch (Exception e) {
        return -7;
    } finally {
        finallyCount = finallyCount + 1;
    }
}

int N = 1000000;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    acc = acc + guarded(i);
}

int cold = 0;
for (int j = 0; j < 1000; j = j + 1) {
    cold = cold + guarded(-1);
}

print("try_catch_finally_hot acc=" + acc + " cold=" + cold + " finally=" + finallyCount);
