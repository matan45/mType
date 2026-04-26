// Per-await suspend/resume cost. Tight loop awaiting a tiny async leaf —
// dominant cost is the continuation/event-loop machinery, not the body.
// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.

import { Int } from "../lib/primitives/Int.mt";

function async compute(int x): Promise<Int> {
    return new Int(x * 2 + 1);
}

function async run(int n): Promise<Int> {
    int total = 0;
    for (int i = 0; i < n; i = i + 1) {
        Int v = await compute(i);
        total = total + v.getValue();
    }
    return new Int(total);
}

int N = 1000000;
Int result = await run(N);
print("async_await_tight_loop total=" + result.getValue());
