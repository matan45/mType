// Test file for O1/O2 optimization - Dead Code Elimination in Finally Blocks
// Tests unreachable code after return statements in finally blocks

import * from "../../lib/exceptions/Exception.mt";

function testFinallyWithReturn(): int {
    try {
        print("Try block");
        return 10;
    } catch (Exception e) {
        print("Catch block");
        return 20;
    } finally {
        print("Finally block");
        return 30;
        print("Dead code in finally");  // Should be removed
        int x = 100;                     // Should be removed
    }

    print("Dead after try-catch-finally");  // Should be removed
    return 999;                              // Should be removed
}

function testFinallyWithThrow(): void {
    try {
        print("Try block");
    } catch (Exception e) {
        print("Catch block");
    } finally {
        print("Finally with throw");
        throw new Exception("Finally exception");
        print("Dead after throw in finally");  // Should be removed
        int y = 200;                            // Should be removed
    }
}

function testNestedTryFinally(): int {
    try {
        try {
            print("Inner try");
            return 1;
        } finally {
            print("Inner finally");
            return 2;
            print("Dead in inner finally");  // Should be removed
        }
        print("Dead after inner try");       // Should be removed
    } finally {
        print("Outer finally");
        return 3;
        print("Dead in outer finally");      // Should be removed
    }

    print("Dead after outer try");           // Should be removed
    return 999;                               // Should be removed
}

function testFinallyOverridesTryReturn(): int {
    try {
        print("Try returns 100");
        return 100;
        print("Dead in try");  // Should be removed
    } finally {
        print("Finally overrides with 200");
        return 200;
        print("Dead in finally");  // Should be removed
    }

    print("Dead after try-finally");  // Should be removed
    return 999;                        // Should be removed
}

function testFinallyOverridesCatchReturn(): int {
    try {
        print("Try throws");
        throw new Exception("Test error");
    } catch (Exception e) {
        print("Catch returns 100");
        return 100;
        print("Dead in catch");  // Should be removed
    } finally {
        print("Finally overrides with 200");
        return 200;
        print("Dead in finally");  // Should be removed
    }

    print("Dead after try-catch-finally");  // Should be removed
    return 999;                              // Should be removed
}

function testFinallyWithBreak(): void {
    for (int i = 0; i < 5; i = i + 1) {
        try {
            print("Try iteration: " + i);
            if (i == 2) {
                throw new Exception("Break error");
            }
        } catch (Exception e) {
            print("Catch iteration: " + i);
        } finally {
            print("Finally with break");
            break;
            print("Dead after break in finally");  // Should be removed
            i = i + 1;                              // Should be removed
        }
        print("Dead after finally in loop");       // Should be removed
    }
}

function testFinallyWithContinue(): void {
    for (int i = 0; i < 3; i = i + 1) {
        try {
            print("Try iteration: " + i);
        } finally {
            print("Finally with continue");
            continue;
            print("Dead after continue in finally");  // Should be removed
            i = i + 10;                                // Should be removed
        }
        print("Dead after finally in loop");          // Should be removed
    }
}

function testFinallyWithoutReturn(): int {
    try {
        print("Try block");
        return 100;
    } finally {
        print("Finally without return - this is reachable");
        int temp = 42;  // This is reachable, not dead code
        print("Finally completes normally");
    }
    // This is dead because try returns
    print("Dead after try-finally");  // Should be removed
    return 999;                        // Should be removed
}

// Entry point
print("=== Testing Finally Block Dead Code Elimination ===");

int result1 = testFinallyWithReturn();
print("Result 1 (finally return): " + result1);

try {
    testFinallyWithThrow();
} catch (Exception e) {
    print("Caught exception from finally");
}

int result3 = testNestedTryFinally();
print("Result 3 (nested finally): " + result3);

int result4 = testFinallyOverridesTryReturn();
print("Result 4 (finally overrides try): " + result4);

int result5 = testFinallyOverridesCatchReturn();
print("Result 5 (finally overrides catch): " + result5);

testFinallyWithBreak();
testFinallyWithContinue();

int result8 = testFinallyWithoutReturn();
print("Result 8 (finally without return): " + result8);

print("Finally Block Dead Code Test Complete!");
