// Two pending promises with different delay durations (1ms vs 3ms),
// awaited sequentially in source order from a single task. The ms gap
// stretches the second pending window but cannot reorder output:
// single-task sequential awaits always settle in source order.
// Pins that source order dominates timing within one task.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Pending Interleaved Settlement Order ===");

function async shortDelay(): Promise<Int> {
    await delay(1);
    return new Int(1);
}

function async longerDelay(): Promise<Int> {
    await delay(3);
    return new Int(3);
}

function async main(): Promise<Int> {
    print("main: awaiting short first");
    Int s = await shortDelay();
    print("main: short returned " + s.getValue());

    print("main: awaiting longer second");
    Int l = await longerDelay();
    print("main: longer returned " + l.getValue());

    print("OK");
    return new Int(s.getValue() + l.getValue());
}

main();
