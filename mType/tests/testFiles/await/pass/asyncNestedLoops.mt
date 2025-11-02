// Test nested loops with async break/continue

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Nested Loops Test ===");

function async shouldSkipInner(int i, int j): Promise<bool> {
    // Skip when i equals j
    return i == j;
}

function async shouldBreakOuter(int i): Promise<bool> {
    // Break outer when i reaches 4
    return i >= 4;
}

function async nestedLoopTest(): Promise<Int> {
    int total = 0;

    for (int i = 1; i <= 5; i = i + 1) {
        print("Outer loop: " + i);

        bool breakOuter = await shouldBreakOuter(i);
        if (breakOuter) {
            print("Breaking outer loop at: " + i);
            break;
        }

        for (int j = 1; j <= 3; j = j + 1) {
            print("  Inner loop: " + j);

            bool skipInner = await shouldSkipInner(i, j);
            if (skipInner) {
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
