// A single pending rejected promise from delayReject(1, ex) stored
// in a Promise<void> variable and awaited twice in sequence inside
// try/catch. Pins that the same pending-rejected promise can be
// observed by two consecutive awaits in one task and that the
// rejection delivers the same exception to both catch blocks.

import * from "../../lib/exceptions/Exception.mt";

print("=== Async Pending Delay Reject Fanout ===");

function async main(): Promise<void> {
    Promise<void> rejected = delayReject(1, new Exception("boom"));

    try {
        await rejected;
        print("first: should not print");
    } catch (Exception e) {
        print("first: caught " + e.getMessage());
    }

    try {
        await rejected;
        print("second: should not print");
    } catch (Exception e) {
        print("second: caught " + e.getMessage());
    }

    print("OK");
}

main();
