// Test try without catch, only finally
import * from "../../lib/exceptions/Exception.mt";

function testTryFinally(): void {
    try {
        print("In try block");
        print("No exception thrown");
    } finally {
        print("In finally block");
    }
}

function testTryFinallyWithException(): void {
    try {
        try {
            print("In inner try");
            throw new Exception("Test exception");
        } finally {
            print("In inner finally");
        }
    } catch (Exception e) {
        print("Caught in outer: " + e.getMessage());
    }
}

function testTryFinallyEarlyReturn(): int {
    try {
        print("In try before return");
        return 42;
    } finally {
        print("In finally after return");
    }
}

function main(): void {
    print("Testing try-finally without catch");

    testTryFinally();
    testTryFinallyWithException();

    int result = testTryFinallyEarlyReturn();
    print("Result: " + result);

    print("Test completed");
}

main();
