// Integration Test 13: Async/Await with Nested Try-Catch
// Tests: Async exception propagation + Nested try-catch blocks

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/exceptions/Exception.mt";

// Exception classes
class AsyncException extends Exception {
   
    constructor(string msg): super(msg) {
    }

    
}

class ValidationException extends AsyncException {
    constructor(string msg) : super(msg) {}
}

class ProcessingException extends AsyncException {
    constructor(string msg) : super(msg) {}
}

// Async functions that may throw
function async validateInput(int value): Promise<Int> {
    print("Validating: " + value);

    if (value < 0) {
        ValidationException ex = new ValidationException("Negative value");
        throw ex;
    }

    return new Int(value);
}

function async processValue(int value): Promise<Int> {
    print("Processing: " + value);

    if (value > 100) {
        ProcessingException ex = new ProcessingException("Value too large");
        throw ex;
    }

    return new Int(value * 2);
}

// Nested try-catch in async function
function async nestedAsyncOperation(int value): Promise<Int> {
    try {
        print("Outer try: " + value);

        try {
            print("Inner try");
            Int validated = await validateInput(value);
            Int processed = await processValue(validated.getValue());
            return processed;

        } catch (ValidationException e) {
            print("Inner catch (Validation): " + e.getMessage());
            return new Int(-1);
        } finally {
            print("Inner finally");
        }

    } catch (ProcessingException e) {
        print("Outer catch (Processing): " + e.getMessage());
        return new Int(-2);
    } catch (AsyncException e) {
        print("Outer catch (Generic): " + e.getMessage());
        return new Int(-3);
    } finally {
        print("Outer finally");
    }
}

// Async function with sequential operations
function async sequentialOperations(int v1, int v2): Promise<Int> {
    try {
        Int r1 = await validateInput(v1);
        print("First validation passed");

        Int r2 = await validateInput(v2);
        print("Second validation passed");

        Int p1 = await processValue(r1.getValue());
        print("First processing passed");

        Int p2 = await processValue(r2.getValue());
        print("Second processing passed");

        return p1 + p2;

    } catch (ValidationException e) {
        print("Caught ValidationException: " + e.getMessage());
        return new Int(0);
    } catch (ProcessingException e) {
        print("Caught ProcessingException: " + e.getMessage());
        return new Int(0);
    } finally {
        print("Sequential operations finally");
    }
}

// Async function with try-catch in loop
function async asyncLoop(int count): Promise<Int> {
    int successCount = 0;

    for (int i = 0; i < count; i = i + 1) {
        try {
            int testValue = (i - 2) * 30;  // Generate test values
            Int result = await processValue(testValue);
            print("Loop " + i + " success: " + result.getValue());
            successCount = successCount + 1;

        } catch (ProcessingException e) {
            print("Loop " + i + " failed: " + e.getMessage());
        }
    }

    return new Int(successCount);
}

// Async function that rethrows
function async rethrowExample(int value): Promise<Int> {
    try {
        Int result = await validateInput(value);
        return result;
    } catch (ValidationException e) {
        print("Caught and rethrowing: " + e.getMessage());
        throw e;  // Rethrow
    }
}

// Async function with nested awaits
function async nestedAwaits(int value): Promise<String> {
    try {
        try {
            Int validated = await validateInput(value);
            print("Inner: Validated");

            Int processed = await processValue(validated.getValue());
            print("Inner: Processed");

            return new String("Success: " + processed.getValue());

        } catch (ProcessingException e) {
            print("Inner catch: " + e.getMessage());
            return new String("Inner handled");
        }

    } catch (ValidationException e) {
        print("Outer catch: " + e.getMessage());
        return new String("Outer handled");
    } finally {
        print("Outer finally executed");
    }
}

// Main async function
function async main(): Promise<void> {
    print("=== Test 13: Async/Await with Nested Try-Catch ===");

    // Test 1: Nested async with valid input
    print("--- Test 1: Valid input ---");
    Int r1 = await nestedAsyncOperation(50);
    print("Result: " + r1.getValue());

    // Test 2: Nested async with invalid input (negative)
    print("--- Test 2: Negative input ---");
    Int r2 = await nestedAsyncOperation(-10);
    print("Result: " + r2.getValue());

    // Test 3: Nested async with too large value
    print("--- Test 3: Too large value ---");
    Int r3 = await nestedAsyncOperation(150);
    print("Result: " + r3.getValue());

    // Test 4: Sequential operations - all valid
    print("--- Test 4: Sequential valid ---");
    Int r4 = await sequentialOperations(10, 20);
    print("Result: " + r4.getValue());

    // Test 5: Sequential operations - second fails
    print("--- Test 5: Sequential with failure ---");
    Int r5 = await sequentialOperations(10, -5);
    print("Result: " + r5.getValue());

    // Test 6: Async loop
    print("--- Test 6: Async loop ---");
    Int r6 = await asyncLoop(5);
    print("Success count: " + r6.getValue());

    // Test 7: Rethrow example
    print("--- Test 7: Rethrow ---");
    try {
        Int r7 = await rethrowExample(-20);
        print("Result: " + r7.getValue());
    } catch (ValidationException e) {
        print("Caught rethrown: " + e.getMessage());
    }

    // Test 8: Nested awaits
    print("--- Test 8: Nested awaits (valid) ---");
    String r8 = await nestedAwaits(25);
    print("Result: " + r8.getValue());

    print("--- Test 8: Nested awaits (too large) ---");
    String r9 = await nestedAwaits(120);
    print("Result: " + r9.getValue());

    print("--- Test 8: Nested awaits (negative) ---");
    String r10 = await nestedAwaits(-5);
    print("Result: " + r10.getValue());

    print("=== Test 13 Complete ===");
    return null;
}

main();
