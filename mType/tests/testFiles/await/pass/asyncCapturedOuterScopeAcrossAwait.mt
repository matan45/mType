// Pins outer-scope capture survival across event-loop suspension.
// An async lambda captures a mutable Int from the enclosing scope,
// awaits delay(1), then mutates the captured field. The caller
// observes the post-mutation state after awaiting the lambda.

import { Int } from "../../lib/primitives/Int.mt";

interface AsyncRunner {
    function async run(): Promise<Int>;
}

print("=== Async Captured Outer Scope Across Await ===");

function async main(): Promise<Int> {
    Int shared = new Int(5);

    AsyncRunner bump = async () -> {
        await delay(1);
        Int next = new Int(shared.getValue() + 100);
        return next;
    };

    print("before: shared=" + shared.getValue());
    Int result = await bump.run();
    print("after: result=" + result.getValue() + " shared=" + shared.getValue());
    print("OK");
    return result;
}

main();
