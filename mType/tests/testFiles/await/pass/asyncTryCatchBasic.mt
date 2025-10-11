import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test 1: Basic async function with try-catch
function async asyncWithTryCatch(): Promise<Int> {
    try {
        print("In try block");
        return new Int(42);
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
        return new Int(-1);
    }
}

// Test 2: Async function that throws in try block
function async asyncThrowInTry(): Promise<String> {
    try {
        print("Before throw");
        Exception e = new Exception("Test exception");
        throw e;
        print("After throw - should not print");
        return new String("success");
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
        return new String("caught");
    }
}

// Test 3: Async function with finally block
function async asyncWithFinally(): Promise<Int> {
    try {
        print("In try");
        return new Int(100);
    } finally {
        print("Finally executed");
    }
}

// Test 4: Async function with try-catch-finally
function async asyncCompleteTryCatchFinally(): Promise<String> {
    try {
        print("Try block start");
        RuntimeException e = new RuntimeException("Runtime error");
        throw e;
    } catch (RuntimeException e) {
        print("Caught: " + e.getMessage());
        return new String("handled");
    } finally {
        print("Finally block executed");
    }
}

function async main(): Promise<void> {
// Run tests
print("=== Test 1: Basic async try-catch ===");
Promise<Int> p1 = asyncWithTryCatch();
Int result1 = await p1;
print("Result: " + result1.toString());

print("");
print("=== Test 2: Async throw in try ===");
Promise<String> p2 = asyncThrowInTry();
String result2 = await p2;
print("Result: " + result2.toString());

print("");
print("=== Test 3: Async with finally ===");
Promise<Int> p3 = asyncWithFinally();
Int result3 = await p3;
print("Result: " + result3.toString());

print("");
print("=== Test 4: Complete try-catch-finally ===");
Promise<String> p4 = asyncCompleteTryCatchFinally();
String result4 = await p4;
print("Result: " + result4.toString());

print("");
print("All async try-catch tests completed");
return null;
}

main();
