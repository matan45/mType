// Test multiple catch blocks for different exceptions
import * from "../../lib/exceptions/Exception.mt";

class CustomException extends Exception {
    constructor(String message) {
        super(message);
    }
}

class AnotherException extends Exception {
    constructor(String message) {
        super(message);
    }
}

function testMultipleCatch(int testCase): void {
    try {
        if (testCase == 1) {
            throw new CustomException("Custom error");
        } else if (testCase == 2) {
            throw new AnotherException("Another error");
        } else {
            throw new Exception("Generic error");
        }
    } catch (CustomException e) {
        print("Caught CustomException: " + e.getMessage());
    } catch (AnotherException e) {
        print("Caught AnotherException: " + e.getMessage());
    } catch (Exception e) {
        print("Caught Exception: " + e.getMessage());
    }
}

function main(): void {
    print("Testing multiple catch blocks");

    testMultipleCatch(1);
    testMultipleCatch(2);
    testMultipleCatch(3);

    print("Test completed");
}

main();
