// Test continue in catch block with finally
import * from "../../lib/exceptions/Exception.mt";

function testContinueInCatch(): void {
    print("Testing continue in catch block");

    for (int i = 0; i < 5; i++) {
        try {
            print("Loop: i=" + i);
            if (i % 2 == 1) {
                throw new Exception("Odd number: " + i);
            }
            print("Even number processed: " + i);
        } catch (Exception e) {
            print("Caught: " + e.getMessage());
            print("Continuing to next iteration");
            continue;
        } finally {
            print("Finally: i=" + i);
        }
        print("After try-catch-finally: i=" + i);
    }
    print("Loop complete");
}

function testContinueInCatchWithNested(): void {
    print("Testing continue in nested loops with catch");

    for (int i = 0; i < 3; i++) {
        print("Outer loop: i=" + i);
        for (int j = 0; j < 4; j++) {
            try {
                print("Inner: i=" + i + ", j=" + j);
                if (j == 2) {
                    throw new Exception("Exception at j=2");
                }
                print("Processing j=" + j);
            } catch (Exception e) {
                print("Inner catch: " + e.getMessage());
                continue;
            } finally {
                print("Inner finally: j=" + j);
            }
            print("After inner try-catch: j=" + j);
        }
        print("Inner loop complete for i=" + i);
    }
}

function testContinueWithMultipleCatches(): void {
    print("Testing continue with multiple catch blocks");

    class CustomException extends Exception {
        constructor(string msg): super(msg) {
    }
    }

    for (int i = 0; i < 6; i++) {
        try {
            print("Iteration: " + i);
            if (i == 1) {
                throw new Exception("Standard exception");
            }
            if (i == 3) {
                throw new CustomException("Custom exception");
            }
            print("No exception for i=" + i);
        } catch (CustomException e) {
            print("Custom catch: " + e.getMessage());
            continue;
        } catch (Exception e) {
            print("Standard catch: " + e.getMessage());
            continue;
        } finally {
            print("Finally: i=" + i);
        }
        print("End of iteration: i=" + i);
    }
}

function testContinueInWhile(): void {
    print("Testing continue in while with catch-finally");

    int count = 0;
    while (count < 5) {
        try {
            print("While: count=" + count);
            count++;
            if (count == 2 || count == 4) {
                throw new Exception("Skip count=" + count);
            }
            print("Processing count=" + count);
        } catch (Exception e) {
            print("Catch: " + e.getMessage());
            continue;
        } finally {
            print("Finally: count=" + count);
        }
        print("End of while body: count=" + count);
    }
}

function main(): void {
    print("Testing continue in catch-finally blocks");

    testContinueInCatch();
    print("---");

    testContinueInCatchWithNested();
    print("---");

    testContinueWithMultipleCatches();
    print("---");

    testContinueInWhile();

    print("Test completed");
}

main();
