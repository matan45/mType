// Pins for-loop iteration variable survival across suspension. After
// await delay(1) mid-body, the loop variable `i` must hold the
// same iteration number used to enter the body, and the loop must
// continue with the correct successor on the next iteration.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Loop Var Across Await ===");

function async main(): Promise<Int> {
    int observed = 0;
    for (int i = 1; i <= 4; i = i + 1) {
        print("pre: i=" + i);
        await delay(1);
        print("post: i=" + i);
        observed = observed + i;
    }
    print("observed=" + observed);
    print("OK");
    return new Int(observed);
}

main();
