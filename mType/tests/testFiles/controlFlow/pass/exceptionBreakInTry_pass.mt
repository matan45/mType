// Test break in try block with finally
import * from "../../lib/exceptions/Exception.mt";

function testBreakInTry(): void {
    print("Testing break in try with finally");

    for (int i = 0; i < 5; i++) {
        try {
            print("Try block iteration: " + i);
            if (i == 3) {
                print("Breaking at iteration: " + i);
                break;
            }
        } finally {
            print("Finally block iteration: " + i);
        }
        print("After try-finally: " + i);
    }
    print("After loop");
}

function testBreakWithException(): void {
    print("Testing break with exception handling");

    for (int i = 0; i < 5; i++) {
        try {
            if (i == 2) {
                throw new Exception("Exception at " + i);
            }
            print("Normal iteration: " + i);
            if (i == 4) {
                break;
            }
        } catch (Exception e) {
            print("Caught: " + e.getMessage());
            continue;
        } finally {
            print("Cleanup iteration: " + i);
        }
        print("End of iteration: " + i);
    }
    print("Loop completed");
}

function testNestedBreakWithFinally(): void {
    print("Testing nested break with finally");

    for (int i = 0; i < 3; i++) {
        try {
            for (int j = 0; j < 3; j++) {
                try {
                    print("Inner loop: i=" + i + " j=" + j);
                    if (j == 2) {
                        break;
                    }
                } finally {
                    print("Inner finally: j=" + j);
                }
            }
        } finally {
            print("Outer finally: i=" + i);
        }
    }
}

function main(): void {
    print("Testing break in try blocks");

    testBreakInTry();
    testBreakWithException();
    testNestedBreakWithFinally();

    print("Test completed");
}

main();
