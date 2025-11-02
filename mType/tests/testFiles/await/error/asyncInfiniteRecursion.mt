// Error: Async infinite recursion should trigger stack overflow

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Infinite Recursion Test ===");

// This will cause stack overflow
function async infiniteRecursion(int n): Promise<Int> {
    print("Recursion depth: " + n);

    // Never reaches base case
    Int result = await infiniteRecursion(n + 1);
    return result;
}

function async main(): Promise<Int> {
    Int result = await infiniteRecursion(0);
    return result;
}

main();
