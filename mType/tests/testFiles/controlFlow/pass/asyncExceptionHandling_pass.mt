// Test exception handling in async functions
// Validates try-catch-finally with async/await control flow

import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";

class AsyncData {
    string status;
    int code;

    public constructor(string s, int c) {
        this.status = s;
        this.code = c;
    }

    public function getStatus(): string {
        return this.status;
    }

    public function getCode(): int {
        return this.code;
    }
}

print("=== Async Exception Handling Test ===");

// Async function that succeeds
function async successfulOperation(): Promise<AsyncData> {
    print("Executing successful operation");
    AsyncData data = new AsyncData("success", 200);
    return data;
}

// Async function that throws
function async failingOperation(): Promise<AsyncData> {
    print("Executing failing operation");
    Exception e = new Exception("Async operation failed");
    throw e;
    AsyncData data = new AsyncData("failed", 500);
    return data;
}

// Test 1: Basic try-catch with async
function async testBasicCatch(): Promise<void> {
    print("");
    print("Test 1: Basic try-catch");
    try {
        AsyncData data = await successfulOperation();
        print("Success: " + data.getStatus() + " (code " + data.getCode() + ")");
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    }
    return null;
}

// Test 2: Catching exceptions from async
function async testCatchException(): Promise<void> {
    print("");
    print("Test 2: Catching async exception");
    try {
        AsyncData data = await failingOperation();
        print("Should not reach here");
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    }
    return null;
}

// Test 3: Finally block with async
function async testFinally(): Promise<void> {
    print("");
    print("Test 3: Finally block");
    try {
        AsyncData data = await successfulOperation();
        print("Operation completed: " + data.getStatus());
    } catch (Exception e) {
        print("Exception: " + e.getMessage());
    } finally {
        print("Finally block executed");
    }
    return null;
}

// Test 4: Exception propagation through async chain
function async innerAsyncCall(): Promise<AsyncData> {
    print("Inner call throwing exception");
    RuntimeException e = new RuntimeException("Inner failure");
    throw e;
    AsyncData data = new AsyncData("inner", 300);
    return data;
}

function async outerAsyncCall(): Promise<AsyncData> {
    print("Outer call awaiting inner");
    AsyncData data = await innerAsyncCall();
    return data;
}

function async testPropagation(): Promise<void> {
    print("");
    print("Test 4: Exception propagation");
    try {
        AsyncData data = await outerAsyncCall();
        print("Should not reach here");
    } catch (RuntimeException e) {
        print("Caught propagated exception: " + e.getMessage());
    } catch (Exception e) {
        print("Caught as base Exception: " + e.getMessage());
    }
    return null;
}

// Test 5: Multiple catch blocks with async
function async testMultipleCatch(): Promise<void> {
    print("");
    print("Test 5: Multiple catch blocks");
    try {
        RuntimeException e = new RuntimeException("Runtime error in async");
        throw e;
    } catch (RuntimeException e) {
        print("Caught RuntimeException: " + e.getMessage());
    } catch (Exception e) {
        print("Caught Exception: " + e.getMessage());
    } finally {
        print("Cleanup in finally");
    }
    return null;
}

// Main function
function async main(): Promise<void> {
    testBasicCatch();
    testCatchException();
    testFinally();
    testPropagation();
    testMultipleCatch();

    print("");
    print("All async exception handling tests complete");
    return null;
}

main();
