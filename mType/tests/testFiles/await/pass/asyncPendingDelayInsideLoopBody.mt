// Pins suspend/resume inside a nested for-loop body. The outer loop
// runs 2 iterations, the inner runs 3, with await delay(1) only in
// the inner body. Distinct from asyncPendingAwaitInLoop.mt because
// the nesting forces the resumed task to re-enter the outer loop's
// remaining iterations across multiple inner suspensions.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Pending Delay Inside Loop Body ===");

function async main(): Promise<Int> {
    int sum = 0;
    for (int i = 1; i <= 2; i = i + 1) {
        for (int j = 1; j <= 3; j = j + 1) {
            await delay(1);
            sum = sum + (i * j);
        }
    }
    print("sum=" + sum);
    print("OK");
    return new Int(sum);
}

main();
