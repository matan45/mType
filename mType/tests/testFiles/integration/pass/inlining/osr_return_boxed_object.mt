// MYT-259: OSR-emitted RETURN_VALUE must preserve identity & field state of a
// freshly-allocated object returned from inside a hot inner loop. Exercises
// the boxed branch of emitOsrPushReturnValueToInterpStack — the returned
// Box's lifetime must outlive the JIT frame's cleanup. If escape analysis
// were to (incorrectly) stack-promote `new Box(...)` because the local
// variable doesn't escape the function body, the returned object would be
// a raw pointer into the JIT's stack arena and the post-RETURN_VALUE read
// of `.payload` would see garbage or crash. EA must keep the box on the
// heap when it escapes via return.

import * from "../../../lib/primitives/Int.mt";

class Box {
    public int payload;
    public constructor(int p) { this.payload = p; }
    public function getPayload(): int { return this.payload; }
}

// Inner loop allocates and returns a Box on early exit. Drives enough
// body→incr / incr→header back-edges per call to trip the OSR threshold.
function findAndWrap(int[] arr, int target): Box? {
    for (int i = 0; i < arr.length; i = i + 1) {
        if (arr[i] == target) {
            return new Box(arr[i] * 2);
        }
    }
    return null;
}

int N = 8;
int[] data = [10, 20, 30, 40, 50, 60, 70, 80];

int hits = 0;
int sumPayload = 0;

// 5000 outer iters × avg 3.5 inner iters per call ≫ inner OSR threshold (500).
for (int j = 0; j < 5000; j = j + 1) {
    int target = data[j % N];
    Box? result = findAndWrap(data, target);
    if (result != null) {
        hits = hits + 1;
        sumPayload = sumPayload + result.getPayload();
    }
}

// Expected: every lookup matches → hits = 5000.
// sumPayload = (10+20+...+80) * 2 * (5000/8) = 360 * 2 * 625 = 450000.
print("osr_return_boxed_object hits=" + hits + " sumPayload=" + sumPayload);
