// Test mutual recursion with async functions

import { Int } from "../../lib/primitives/Int.mt";
import { Bool } from "../../lib/primitives/Bool.mt";

print("=== Async Mutual Recursion Test ===");

// Two async functions calling each other
function async isEven(int n): Promise<Bool> {
    if (n == 0) {
        print("isEven base case: true");
        return new Bool(true);
    }
    print("isEven(" + n + ") calling isOdd(" + (n - 1) + ")");
    Bool result = await isOdd(n - 1);
    return result;
}

function async isOdd(int n): Promise<Bool> {
    if (n == 0) {
        print("isOdd base case: false");
        return new Bool(false);
    }
    print("isOdd(" + n + ") calling isEven(" + (n - 1) + ")");
    Bool result = await isEven(n - 1);
    return result;
}

function async main(): Promise<Int> {
    Bool result1 = await isEven(4);
    print("isEven(4) = " + result1.getValue());

    Bool result2 = await isOdd(5);
    print("isOdd(5) = " + result2.getValue());

    Bool result3 = await isEven(7);
    print("isEven(7) = " + result3.getValue());

    return new Int(0);
}

main();
