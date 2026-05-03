// Regression test for MYT-265.
// Pins repeated suspend/resume across sequential pending awaits in one task,
// using delay(ms) to force each await onto the suspend branch.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Pending Await In Loop ===");

function async main(): Promise<Int> {
    int sum = 0;

    for (int i = 1; i <= 3; i = i + 1) {
        print("loop: before await " + i);
        await delay(1);
        print("loop: after await " + i);
        sum = sum + i;
    }

    print("main: sum=" + sum);
    print("OK");
    return new Int(sum);
}

main();
