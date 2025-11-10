// Test async in anonymous/nested scope

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Anonymous Block Test ===");

function async getValue(int x): Promise<Int> {
    print("Getting value: " + x);
    return new Int(x * 2);
}

function async processInBlock(): Promise<Int> {
    print("Starting block processing");
    int total = 0;

    // Nested block with async operations
    {
        print("Entering nested block 1");
        Int val1 = await getValue(5);
        total = total + val1.getValue();
        print("Block 1 total: " + total);
    }

    {
        print("Entering nested block 2");
        Int val2 = await getValue(10);
        total = total + val2.getValue();
        print("Block 2 total: " + total);
    }

    print("All blocks completed");
    return new Int(total);
}

function async main(): Promise<Int> {
    Int result = await processInBlock();
    print("Final result: " + result);
    return result;
}

main();
