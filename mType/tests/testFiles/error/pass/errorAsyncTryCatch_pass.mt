// Test: Async try-catch exception handling
// Expected: All catch blocks handle their respective exceptions correctly
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Custom exception types
class ValidationException extends Exception {
    public constructor(string msg) {
        super(msg);
    }
}

class NetworkException extends RuntimeException {
    public constructor(string msg) {
        super(msg);
    }
}

// Test 1: Basic try-catch
function async basicTryCatch(bool shouldThrow): Promise<Int> {
    try {
        print("Executing operation");
        if (shouldThrow) {
            Exception e = new Exception("Basic error");
            throw e;
        }
        return new Int(100);
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
        return new Int(-1);
    }
}

// Test 2: Specific exception type
function async specificCatch(): Promise<String> {
    try {
        print("Validating data");
        ValidationException e = new ValidationException("Invalid input");
        throw e;
        return new String("valid");
    } catch (ValidationException e) {
        print("Validation failed: " + e.getMessage());
        return new String("invalid");
    }
}

// Test 3: Multiple operations in try block
function async multipleOperations(): Promise<Int> {
    try {
        print("Step 1: Initialize");
        Int value = new Int(10);

        print("Step 2: Process");
        Int doubled = new Int(value.toInt() * 2);

        print("Step 3: Throw error");
        NetworkException e = new NetworkException("Connection failed");
        throw e;

        print("Step 4: Never reached");
        return doubled;
    } catch (NetworkException e) {
        print("Network error: " + e.getMessage());
        return new Int(0);
    }
}

// Test 4: Nested try-catch
function async nestedTryCatch(): Promise<String> {
    try {
        print("Outer try");
        try {
            print("Inner try");
            Exception e = new Exception("Inner exception");
            throw e;
        } catch (Exception e) {
            print("Inner catch: " + e.getMessage());
            RuntimeException re = new RuntimeException("Outer exception");
            throw re;
        }
        return new String("success");
    } catch (RuntimeException e) {
        print("Outer catch: " + e.getMessage());
        return new String("handled");
    }
}

function async main(): Promise<void> {
    print("=== Test 1: Basic try-catch (no throw) ===");
    Promise<Int> p1 = basicTryCatch(false);
    Int result1 = await p1;
    print("Result: " + result1.toString());

    print("");
    print("=== Test 2: Basic try-catch (with throw) ===");
    Promise<Int> p2 = basicTryCatch(true);
    Int result2 = await p2;
    print("Result: " + result2.toString());

    print("");
    print("=== Test 3: Specific exception type ===");
    Promise<String> p3 = specificCatch();
    String result3 = await p3;
    print("Result: " + result3.toString());

    print("");
    print("=== Test 4: Multiple operations ===");
    Promise<Int> p4 = multipleOperations();
    Int result4 = await p4;
    print("Result: " + result4.toString());

    print("");
    print("=== Test 5: Nested try-catch ===");
    Promise<String> p5 = nestedTryCatch();
    String result5 = await p5;
    print("Result: " + result5.toString());

    print("");
    print("Async try-catch test completed");
    return null;
}

main();
