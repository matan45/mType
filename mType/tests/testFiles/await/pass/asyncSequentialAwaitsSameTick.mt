// Regression test for MYT-114.
// Exercises the resume -> re-await -> resume loop that flows through
// checkCompletedPromises within a single tick. Each await suspends the task,
// the fulfilled promise is observed by checkCompletedPromises, the task is
// moved back to readyQueue and runs to the next await. This is the pattern
// the original tick deadlock made impossible on any settling path.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Sequential Awaits Same Tick ===");

function async step(int n): Promise<Int> {
    return new Int(n);
}

function async driver(): Promise<Int> {
    print("driver: before step 1");
    Int a = await step(1);
    print("driver: after step 1 value=" + a);

    print("driver: before step 2");
    Int b = await step(2);
    print("driver: after step 2 value=" + b);

    print("driver: before step 3");
    Int c = await step(3);
    print("driver: after step 3 value=" + c);

    int sum = a.getValue() + b.getValue() + c.getValue();
    return new Int(sum);
}

function async main(): Promise<Int> {
    Int total = await driver();
    print("main: total=" + total);
    print("OK");
    return total;
}

main();
