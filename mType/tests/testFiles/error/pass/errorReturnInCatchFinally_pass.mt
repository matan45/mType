// Test return in catch block with finally execution
import * from "../../lib/exceptions/Exception.mt";

function testReturnInCatch(): int {
    try {
        print("Try block - throwing exception");
        throw new Exception("Test exception");
    } catch (Exception e) {
        print("Catch block - returning value");
        return 100;
    } finally {
        print("Finally block executed after catch return");
    }
}

function testReturnInCatchOverridesTry(): int {
    try {
        print("Try block - returning 10");
        return 10;
    } catch (Exception e) {
        print("Catch block - should not execute");
        return 20;
    } finally {
        print("Finally after try return");
    }
}

function testReturnInCatchWithNestedTry(): int {
    try {
        try {
            print("Inner try - throwing");
            throw new Exception("Inner exception");
        } catch (Exception e) {
            print("Inner catch - returning from here");
            return 42;
        } finally {
            print("Inner finally");
        }
    } catch (Exception e) {
        print("Outer catch - should not reach");
        return 99;
    } finally {
        print("Outer finally");
    }
}

class CustomException extends Exception {
        constructor(string msg): super(msg) {
    }
    }

function testReturnInMultipleCatches(): int {
    

    try {
        print("Throwing custom exception");
        throw new CustomException("Custom error");
    } catch (CustomException e) {
        print("Custom catch - returning 200");
        return 200;
    } catch (Exception e) {
        print("General catch - should not reach");
        return 300;
    } finally {
        print("Finally after custom catch");
    }
}

function testFinallyDoesNotOverrideCatchReturn(): int {
    try {
        print("Throwing for finally test");
        throw new Exception("Error");
    } catch (Exception e) {
        print("Catch returning 500");
        return 500;
    } finally {
        print("Finally - no return here");
        int temp = 999;
    }
}

function testReturnInCatchAfterProcessing(): int {
    try {
        print("Processing data");
        int[] data = new int[11];
        data[10] = 5;
        return 1;
    } catch (Exception e) {
        print("Error caught: " + e.getMessage());
        print("Returning error code");
        return -1;
    } finally {
        print("Cleanup in finally");
    }
}

function main(): void {
    print("Testing return in catch with finally");

    int result1 = testReturnInCatch();
    print("Result 1: " + result1);
    print("---");

    int result2 = testReturnInCatchOverridesTry();
    print("Result 2: " + result2);
    print("---");

    int result3 = testReturnInCatchWithNestedTry();
    print("Result 3: " + result3);
    print("---");

    int result4 = testReturnInMultipleCatches();
    print("Result 4: " + result4);
    print("---");

    int result5 = testFinallyDoesNotOverrideCatchReturn();
    print("Result 5: " + result5);
    print("---");

    int result6 = testReturnInCatchAfterProcessing();
    print("Result 6: " + result6);

    print("Test completed");
}

main();
