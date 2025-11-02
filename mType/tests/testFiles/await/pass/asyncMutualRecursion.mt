// Test mutual recursion with async functions

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Mutual Recursion Test ===");

// Two async functions calling each other
function async isEven(int n): Promise<bool> {
    if (n == 0) {
        print("isEven base case: true");
        return true;
    }
    print("isEven(" + n + ") calling isOdd(" + (n - 1) + ")");
    bool result = await isOdd(n - 1);
    return result;
}

function async isOdd(int n): Promise<bool> {
    if (n == 0) {
        print("isOdd base case: false");
        return false;
    }
    print("isOdd(" + n + ") calling isEven(" + (n - 1) + ")");
    bool result = await isEven(n - 1);
    return result;
}

function async main(): Promise<Int> {
    bool result1 = await isEven(4);
    print("isEven(4) = " + result1);

    bool result2 = await isOdd(5);
    print("isOdd(5) = " + result2);

    bool result3 = await isEven(7);
    print("isEven(7) = " + result3);

    return new Int(0);
}

main();
