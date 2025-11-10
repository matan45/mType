// Test throwing new exception while handling caught exception
import * from "../../lib/exceptions/Exception.mt";

class FirstException extends Exception {
    constructor(string message): super(message) {
    }
}

class SecondException extends Exception {
    constructor(string message): super(message) {
    }
}

function testThrowInCatch(): void {
    try {
        try {
            print("Throwing first exception");
            throw new FirstException("First error");
        } catch (FirstException e) {
            print("Caught first exception: " + e.getMessage());
            print("Throwing second exception in catch");
            throw new SecondException("Second error from catch block");
        }
    } catch (SecondException e) {
        print("Caught second exception: " + e.getMessage());
    }
}

function testThrowInCatchWithFinally(): void {
    try {
        try {
            print("Throwing exception for finally test");
            throw new FirstException("Original error");
        } catch (FirstException e) {
            print("In catch, about to throw new exception");
            throw new SecondException("Replacement error");
        } finally {
            print("Finally block executed");
        }
    } catch (SecondException e) {
        print("Outer catch: " + e.getMessage());
    }
}

function testNestedThrowInCatch(): void {
    try {
        try {
            try {
                print("Level 3 throw");
                throw new FirstException("Level 3");
            } catch (FirstException e) {
                print("Level 2 catch, rethrowing as SecondException");
                throw new SecondException("Level 2: " + e.getMessage());
            }
        } catch (SecondException e) {
            print("Level 1 catch, rethrowing as Exception");
            throw new Exception("Level 1: " + e.getMessage());
        }
    } catch (Exception e) {
        print("Final catch: " + e.getMessage());
    }
}

function main(): void {
    print("Testing exception in catch block");

    testThrowInCatch();
    print("---");

    testThrowInCatchWithFinally();
    print("---");

    testNestedThrowInCatch();

    print("Test completed");
}

main();
