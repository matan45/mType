// Test nested loops with async break/continue

import { Int } from "../../lib/primitives/Int.mt";
import { Bool } from "../../lib/primitives/Bool.mt";

print("=== Async Nested Loops Test ===");

function async shouldSkipInner(int i, int j): Promise<Bool> {
    // Skip when i equals j
    return new Bool(i == j);
}

function async shouldBreakOuter(int i): Promise<Bool> {
    // Break outer when i reaches 4
    return new Bool(i >= 4);
}

function async nestedLoopTest(): Promise<Int> {
    int total = 0;

    for (int i = 1; i <= 5; i = i + 1) {
        print("Outer loop: " + i);

        Bool breakOuter = await shouldBreakOuter(i);
        if (breakOuter.getValue()) {
            print("Breaking outer loop at: " + i);
            break;
        }

        for (int j = 1; j <= 3; j = j + 1) {
            print("  Inner loop: " + j);

            Bool skipInner = await shouldSkipInner(i, j);
            if (skipInner.getValue()) {
                print("  Skipping inner: " + i + "," + j);
                continue;
            }

            int value = i * j;
            total = total + value;
            print("  Added " + value + ", total: " + total);
        }
    }

    print("Final total: " + total);
    return new Int(total);
}

function async main(): Promise<Int> {
    Int result = await nestedLoopTest();
    print("Result: " + result);
    return result;
}

main();
