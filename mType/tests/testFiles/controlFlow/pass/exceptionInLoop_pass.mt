// Test exception with continue/break in loop
import * from "../../lib/exceptions/Exception.mt";

function testContinueInLoop(): void {
    print("Testing continue in loop with exceptions");

    for (int i = 0; i < 5; i++) {
        try {
            if (i == 2) {
                throw new Exception("Skip iteration " + i);
            }
            print("Processing: " + i);
        } catch (Exception e) {
            print("Caught: " + e.getMessage());
            continue;
        }
        print("After try-catch: " + i);
    }
}

function testBreakInLoop(): void {
    print("Testing break in loop with exceptions");

    int count = 0;
    while (count < 10) {
        try {
            count = count + 1;
            if (count == 4) {
                throw new Exception("Breaking at " + count);
            }
            print("Count: " + count);
        } catch (Exception e) {
            print("Caught: " + e.getMessage());
            break;
        }
    }
    print("Final count: " + count);
}

function testExceptionWithFinally(): void {
    print("Testing exception with finally in loop");

    for (int i = 0; i < 3; i++) {
        try {
            if (i == 1) {
                continue;
            }
            print("Iteration: " + i);
        } finally {
            print("Finally for iteration: " + i);
        }
    }
}

function main(): void {
    print("Testing exceptions in loops");

    testContinueInLoop();
    testBreakInLoop();
    testExceptionWithFinally();

    print("Test completed");
}

main();
