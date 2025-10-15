// Test file for O1/O2 optimization - Dead Code Elimination in Catch Blocks
// Tests unreachable code after return/throw statements in catch blocks

import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/NullPointerException.mt";

function testCatchWithReturn(): string {
    try {
        throw new Exception("Test error");
    } catch (Exception e) {
        print("Caught exception");
        return "error handled";
        print("Dead code in catch");  // Should be removed
        int x = 100;                   // Should be removed
    }

    print("Unreachable after catch"); // Should be removed
    return "never reached";            // Should be removed
}

function testCatchWithThrow(): void {
    try {
        throw new Exception("Test error");
    } catch (Exception e) {
        print("Rethrowing exception");
        throw e;
        print("Dead after throw in catch");  // Should be removed
        int y = 200;                          // Should be removed
    }
}

function testNestedTryCatch(): int {
    try {
        try {
            throw new Exception("Inner exception");
        } catch (Exception e) {
            print("Inner catch");
            return 42;
            print("Dead in inner catch");  // Should be removed
        }
        print("Dead after inner try");     // Should be removed
    } catch (Exception e) {
        print("Outer catch (unreachable)");
        return -1;
    }

    print("Dead after outer try");         // Should be removed
    return 999;                             // Should be removed
}

function testMultipleCatchBlocks(int code): string {
    try {
        if (code == 1) {
            throw new NullPointerException("Null error");
        } else {
            throw new Exception("General error");
        }
    } catch (NullPointerException e) {
        print("Caught NullPointerException");
        return "null error";
        print("Dead in first catch");  // Should be removed
    } catch (Exception e) {
        print("Caught Exception");
        return "general error";
        print("Dead in second catch"); // Should be removed
    }

    print("Dead after all catches");   // Should be removed
    return "never";                     // Should be removed
}

function testCatchWithBreak(): void {
    for (int i = 0; i < 5; i = i + 1) {
        try {
            if (i == 2) {
                throw new Exception("Break error");
            }
        } catch (Exception e) {
            print("Breaking from catch");
            break;
            print("Dead after break in catch");  // Should be removed
            i = i + 1;                            // Should be removed
        }
        print("Loop iteration: " + i);
    }
}

function testCatchWithContinue(): void {
    for (int i = 0; i < 5; i = i + 1) {
        try {
            if (i % 2 == 0) {
                throw new Exception("Continue error");
            }
            print("No exception for i=" + i);
        } catch (Exception e) {
            print("Continuing from catch");
            continue;
            print("Dead after continue in catch");  // Should be removed
            i = i + 10;                              // Should be removed
        }
        print("After catch for i=" + i);
    }
}

// Entry point
string result1 = testCatchWithReturn();
print("Result 1: " + result1);

try {
    testCatchWithThrow();
} catch (Exception e) {
    print("Caught rethrown exception");
}

int result3 = testNestedTryCatch();
print("Result 3: " + result3);

string result4 = testMultipleCatchBlocks(1);
print("Result 4: " + result4);

testCatchWithBreak();
testCatchWithContinue();

print("Catch Block Dead Code Test Complete!");
