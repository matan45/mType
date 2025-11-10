// Test break in try block with finally execution
import * from "../../lib/exceptions/Exception.mt";

function testBreakInTry(): void {
    print("Testing break in try with finally");

    for (int i = 0; i < 5; i++) {
        try {
            print("Loop iteration: " + i);
            if (i == 2) {
                print("Breaking at i=" + i);
                break;
            }
            print("End of try block: " + i);
        } finally {
            print("Finally executed for i=" + i);
        }
    }
    print("After loop");
}

function testBreakInNestedTry(): void {
    print("Testing break in nested try-finally");

    for (int i = 0; i < 3; i++) {
        try {
            print("Outer try: " + i);
            for (int j = 0; j < 4; j++) {
                try {
                    print("Inner try: i=" + i + ", j=" + j);
                    if (j == 2) {
                        print("Breaking inner loop");
                        break;
                    }
                } finally {
                    print("Inner finally: j=" + j);
                }
            }
            print("After inner loop");
        } finally {
            print("Outer finally: i=" + i);
        }
    }
}

function testBreakWithException(): void {
    print("Testing break with exception handling");

    for (int i = 0; i < 4; i++) {
        try {
            print("Iteration: " + i);
            if (i == 1) {
                throw new Exception("Exception at i=1");
            }
            if (i == 3) {
                print("Breaking at i=3");
                break;
            }
        } catch (Exception e) {
            print("Caught: " + e.getMessage());
        } finally {
            print("Finally: i=" + i);
        }
    }
    print("Loop finished");
}

function testBreakInWhileTryFinally(): void {
    print("Testing break in while with try-finally");

    int count = 0;
    while (count < 10) {
        try {
            print("While iteration: " + count);
            count++;
            if (count == 5) {
                print("Breaking while loop");
                break;
            }
        } finally {
            print("While finally: count=" + count);
        }
    }
    print("While loop ended, count=" + count);
}

function main(): void {
    print("Testing break in try-finally blocks");

    testBreakInTry();
    print("---");

    testBreakInNestedTry();
    print("---");

    testBreakWithException();
    print("---");

    testBreakInWhileTryFinally();

    print("Test completed");
}

main();
