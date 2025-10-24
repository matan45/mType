// Test finally block with return statement overriding try/catch returns

function testFinallyReturn1(): int {
    try {
        print("In try block");
        return 10;
    } finally {
        print("In finally block");
        return 20;  // This overrides the try return
    }
}

function testFinallyReturn2(): int {
    try {
        print("In try block 2");
        throw new Exception("Test exception");
    } catch (Exception e) {
        print("In catch block");
        return 30;
    } finally {
        print("In finally block 2");
        return 40;  // This overrides the catch return
    }
}

function testFinallyNoReturn(): int {
    int result = 0;

    try {
        print("In try block 3");
        result = 50;
        return result;
    } finally {
        print("In finally block 3");
        result = 60;  // Modifies variable but doesn't override return
    }
}

function main(): void {
    print("Testing finally overrides return");

    int result1 = testFinallyReturn1();
    print("Result 1: " + result1);

    int result2 = testFinallyReturn2();
    print("Result 2: " + result2);

    int result3 = testFinallyNoReturn();
    print("Result 3: " + result3);

    print("Finally override test completed");
}

main();
