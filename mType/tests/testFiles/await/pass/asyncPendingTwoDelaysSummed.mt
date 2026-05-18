// Pins back-to-back suspend/resume in a single task: two sequential
// await delay(1) calls each return an Int, summed and printed. Forces
// executeAwait's suspend branch twice on the same task without an
// intervening synchronous settlement.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Pending Two Delays Summed ===");

function async firstAfterDelay(): Promise<Int> {
    await delay(1);
    return new Int(11);
}

function async secondAfterDelay(): Promise<Int> {
    await delay(1);
    return new Int(31);
}

function async main(): Promise<Int> {
    print("main: before first await");
    Int a = await firstAfterDelay();
    print("main: got a=" + a.getValue());

    print("main: before second await");
    Int b = await secondAfterDelay();
    print("main: got b=" + b.getValue());

    int total = a.getValue() + b.getValue();
    print("main: total=" + total);
    print("OK");
    return new Int(total);
}

main();
