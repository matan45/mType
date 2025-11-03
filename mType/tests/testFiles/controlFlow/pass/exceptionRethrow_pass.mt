// Test throw in catch block (rethrow)
import * from "../../lib/exceptions/Exception.mt";

class CustomException extends Exception {
    constructor(string message): super(message) {
    }
}

function innerFunction(): void {
    print("Inner function throwing");
    throw new Exception("Original exception");
}

function middleFunction(): void {
    try {
        innerFunction();
    } catch (Exception e) {
        print("Middle caught: " + e.getMessage());
        throw new CustomException("Wrapped: " + e.getMessage());
    }
}

function testRethrow(): void {
    try {
        middleFunction();
    } catch (CustomException e) {
        print("Outer caught: " + e.getMessage());
    }
}

function testDirectRethrow(): void {
    try {
        try {
            throw new Exception("Direct rethrow test");
        } catch (Exception e) {
            print("Caught and rethrowing: " + e.getMessage());
            throw e;
        }
    } catch (Exception e) {
        print("Final catch: " + e.getMessage());
    }
}

function main(): void {
    print("Testing exception rethrow");

    testRethrow();
    testDirectRethrow();

    print("Test completed");
}

main();
