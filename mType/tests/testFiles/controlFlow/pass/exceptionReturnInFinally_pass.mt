// Test return in finally block
import * from "../../lib/exceptions/Exception.mt";

function testReturnInFinallyOverridesTry(): int {
    try {
        print("Try block");
        return 10;
    } finally {
        print("Finally block");
        return 20;
    }
}

function testReturnInFinallyOverridesCatch(): int {
    try {
        print("Throwing exception");
        throw new Exception("Test");
    } catch (Exception e) {
        print("Catch block");
        return 30;
    } finally {
        print("Finally with return");
        return 40;
    }
}

function testReturnInFinallyOverridesException(): int {
    try {
        print("Throwing uncaught exception");
        throw new Exception("Uncaught");
    } finally {
        print("Finally suppresses exception");
        return 50;
    }
}

function testMultipleFinallyReturns(): int {
    try {
        try {
            print("Inner try");
            return 60;
        } finally {
            print("Inner finally");
            return 70;
        }
    } finally {
        print("Outer finally");
        return 80;
    }
}

function testFinallyWithoutReturn(): int {
    try {
        print("Try with return");
        return 90;
    } finally {
        print("Finally without return");
        int temp = 100;
    }
}

function main(): void {
    print("Testing return in finally blocks");

    int result1 = testReturnInFinallyOverridesTry();
    print("Result 1: " + result1);

    int result2 = testReturnInFinallyOverridesCatch();
    print("Result 2: " + result2);

    int result3 = testReturnInFinallyOverridesException();
    print("Result 3: " + result3);

    int result4 = testMultipleFinallyReturns();
    print("Result 4: " + result4);

    int result5 = testFinallyWithoutReturn();
    print("Result 5: " + result5);

    print("Test completed");
}

main();
