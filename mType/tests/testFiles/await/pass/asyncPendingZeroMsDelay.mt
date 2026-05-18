// Pins delay(0) still routing through executeAwait's suspend branch.
// The AsyncPromiseValue is created in PENDING state before scheduling,
// so a 0ms delay never collapses to the synchronously-fulfilled fast
// path; the event loop must drain the delayed-task queue first.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Pending Zero Ms Delay ===");

function async main(): Promise<Int> {
    print("main: before zero-ms await");
    await delay(0);
    print("main: after zero-ms await");
    print("OK");
    return new Int(0);
}

main();
