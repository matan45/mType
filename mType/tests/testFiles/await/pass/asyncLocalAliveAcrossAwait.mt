// Pins ExecutionSnapshot fidelity for primitive locals across a real
// event-loop suspension. Several locals (int, string, Int box) are
// declared before await delay(1); after resume they must hold the
// same values they had pre-suspension.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Local Alive Across Await ===");

function async main(): Promise<Int> {
    int x = 7;
    string label = "answer";
    Int boxed = new Int(35);

    print("before: x=" + x + " label=" + label + " boxed=" + boxed.getValue());
    await delay(1);
    print("after: x=" + x + " label=" + label + " boxed=" + boxed.getValue());

    int product = x * boxed.getValue();
    print("product=" + product);
    print("OK");
    return new Int(product);
}

main();
