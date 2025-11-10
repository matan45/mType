// Test continue in catch block with finally
import * from "../../lib/exceptions/Exception.mt";

function testContinueInCatch(): void {
    print("Testing continue in catch block");

    for (int i = 0; i < 5; i++) {
        try {
            if (i % 2 == 0) {
                throw new Exception("Skip even: " + i);
            }
            print("Processing odd: " + i);
        } catch (Exception e) {
            print("Caught: " + e.getMessage());
            continue;
        } finally {
            print("Finally for iteration: " + i);
        }
        print("After try-catch-finally: " + i);
    }
    print("Loop completed");
}

function testContinueWithMultipleCatch(): void {
    print("Testing continue with multiple catch blocks");

    for (int i = 0; i < 4; i++) {
        try {
            if (i == 1) {
                throw new Exception("Exception at 1");
            }
            print("Normal execution: " + i);
        } catch (Exception e) {
            print("Caught exception: " + e.getMessage());
            if (i == 1) {
                continue;
            }
        } finally {
            print("Cleanup: " + i);
        }
        print("End of iteration: " + i);
    }
}

function testNestedContinue(): void {
    print("Testing nested continue with finally");

    for (int i = 0; i < 3; i++) {
        try {
            for (int j = 0; j < 3; j++) {
                try {
                    if (j == 1) {
                        throw new Exception("Skip j=1");
                    }
                    print("i=" + i + " j=" + j);
                } catch (Exception e) {
                    print("Inner catch: " + e.getMessage());
                    continue;
                } finally {
                    print("Inner finally: j=" + j);
                }
                print("After inner try: j=" + j);
            }
        } finally {
            print("Outer finally: i=" + i);
        }
    }
}

function main(): void {
    print("Testing continue in catch blocks");

    testContinueInCatch();
    testContinueWithMultipleCatch();
    testNestedContinue();

    print("Test completed");
}

main();
