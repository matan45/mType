// Test empty catch block - should be allowed (silent exception swallowing)
import * from "../../lib/exceptions/Exception.mt";

function testEmptyCatch(): void {
    print("Testing empty catch block");

    try {
        print("Throwing exception");
        throw new Exception("This will be silently swallowed");
    } catch (Exception e) {
        // Empty catch - silently swallow the exception
    }

    print("Execution continues after empty catch");
}

function testEmptyCatchWithFinally(): void {
    print("Testing empty catch with finally");

    try {
        print("Throwing exception with finally");
        throw new Exception("Swallowed with cleanup");
    } catch (Exception e) {
        // Empty catch
    } finally {
        print("Finally block executes");
    }

    print("After empty catch with finally");
}

function testMultipleEmptyCatches(): void {
    print("Testing multiple empty catch blocks");

    class CustomException extends Exception {
        constructor(String msg) {
            super(msg);
        }
    }

    try {
        print("Throwing custom exception");
        throw new CustomException("Silently handled");
    } catch (CustomException e) {
        // Empty custom catch
    } catch (Exception e) {
        // Empty general catch
    }

    print("After multiple empty catches");
}

function testEmptyCatchInLoop(): void {
    print("Testing empty catch in loop");

    for (int i = 0; i < 3; i++) {
        try {
            print("Iteration: " + i);
            if (i == 1) {
                throw new Exception("Exception at i=1");
            }
        } catch (Exception e) {
            // Silently continue
        }
    }

    print("Loop completed");
}

function testEmptyCatchNoVariable(): void {
    print("Testing empty catch without variable");

    try {
        print("Throwing exception without catch variable");
        throw new Exception("No variable needed");
    } catch (Exception) {
        // Empty catch, no exception variable
    }

    print("After catch without variable");
}

function main(): void {
    print("Testing empty catch blocks");

    testEmptyCatch();
    print("---");

    testEmptyCatchWithFinally();
    print("---");

    testMultipleEmptyCatches();
    print("---");

    testEmptyCatchInLoop();
    print("---");

    testEmptyCatchNoVariable();

    print("Test completed");
}

main();
