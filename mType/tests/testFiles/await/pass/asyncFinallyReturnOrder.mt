// Test that finally block executes BEFORE return value is sent
// This is critical async/await behavior - matches JavaScript/Python
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

function async finallyBeforeReturn(): Promise<String> {
    try {
        print("1. Try block");
        return new String("from try");
    } finally {
        print("2. Finally block - executes BEFORE return");
    }
    print("3. After finally - NEVER executes (unreachable)");
}

function async finallyOverridesReturn(): Promise<String> {
    try {
        print("1. Try block returns 'first'");
        return new String("first");
    } finally {
        print("2. Finally block returns 'second' - OVERRIDES try return!");
        return new String("second");  // Finally return overrides try return
    }
}

function async finallyAfterCatch(): Promise<Int> {
    try {
        print("1. Try block throws");
        Exception e = new Exception("Test error");
        throw e;
    } catch (Exception e) {
        print("2. Catch block catches and returns 100");
        return new Int(100);
    } finally {
        print("3. Finally executes BEFORE catch return");
    }
}

function async finallyWithAwait(): Promise<String> {
    try {
        print("1. Try with await");
        Promise<String> innerPromise = createPromise("result");
        String value = await innerPromise;
        print("2. After await: " + value.toString());
        return value;
    } finally {
        print("3. Finally block runs before return");
    }
}

// Helper function to create a resolved promise
function async createPromise(string value): Promise<String> {
    return new String(value);
}

function async main(): Promise<void> {
// Run tests
print("=== Test 1: Finally before return ===");
Promise<String> p1 = finallyBeforeReturn();
String result1 = await p1;
print("Returned: " + result1.toString());
print("");

print("=== Test 2: Finally overrides return ===");
Promise<String> p2 = finallyOverridesReturn();
String result2 = await p2;
print("Returned: " + result2.toString() + " (should be 'second')");
print("");

print("=== Test 3: Finally after catch ===");
Promise<Int> p3 = finallyAfterCatch();
Int result3 = await p3;
print("Returned: " + result3.toString());
print("");

print("=== Test 4: Finally with await ===");
Promise<String> p4 = finallyWithAwait();
String result4 = await p4;
print("Returned: " + result4.toString());
print("");

print("All finally-return-order tests completed");
return null;
}

main();
