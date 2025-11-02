// Test: Unhandled exception in async function
// Expected: Exception propagates through promise chain correctly
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Async function that throws without catching
function async throwUnhandled(): Promise<Int> {
    print("Starting async operation");
    RuntimeException e = new RuntimeException("Unhandled async error");
    throw e;
    return new Int(42);  // Never reached
}

// Caller that handles the exception
function async handleUnhandled(): Promise<String> {
    try {
        print("Calling function that throws");
        Promise<Int> p = throwUnhandled();
        Int result = await p;
        print("Should not reach here: " + result.toString());
        return new String("failure");
    } catch (RuntimeException e) {
        print("Caught unhandled exception: " + e.getMessage());
        return new String("handled");
    }
}

// Multiple levels of propagation
function async level3(): Promise<Int> {
    print("Level 3: Throwing exception");
    Exception e = new Exception("Deep exception");
    throw e;
    return new Int(0);
}

function async level2(): Promise<Int> {
    print("Level 2: Propagating");
    Promise<Int> p = level3();
    Int result = await p;
    return result;
}

function async level1(): Promise<String> {
    print("Level 1: Catching");
    try {
        Promise<Int> p = level2();
        Int value = await p;
        return new String("unexpected");
    } catch (Exception e) {
        print("Caught at level 1: " + e.getMessage());
        return new String("propagated");
    }
}

function async main(): Promise<void> {
    print("=== Test 1: Basic unhandled exception ===");
    Promise<String> p1 = handleUnhandled();
    String result1 = await p1;
    print("Result: " + result1.toString());

    print("");
    print("=== Test 2: Multi-level propagation ===");
    Promise<String> p2 = level1();
    String result2 = await p2;
    print("Result: " + result2.toString());

    print("");
    print("Unhandled exception test completed");
    return null;
}

main();
