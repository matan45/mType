// Test that finally block executes BEFORE return value is sent
// This is critical async/await behavior - matches JavaScript/Python

async function finallyBeforeReturn(): Promise<string> {
    try {
        print("1. Try block");
        return "from try";
    } finally {
        print("2. Finally block - executes BEFORE return");
    }
    print("3. After finally - NEVER executes (unreachable)");
}

async function finallyOverridesReturn(): Promise<string> {
    try {
        print("1. Try block returns 'first'");
        return "first";
    } finally {
        print("2. Finally block returns 'second' - OVERRIDES try return!");
        return "second";  // Finally return overrides try return
    }
}

async function finallyAfterCatch(): Promise<int> {
    try {
        print("1. Try block throws");
        Exception e = new Exception("Test error");
        throw e;
    } catch (Exception e) {
        print("2. Catch block catches and returns 100");
        return 100;
    } finally {
        print("3. Finally executes BEFORE catch return");
    }
}

async function finallyWithAwait(): Promise<string> {
    try {
        print("1. Try with await");
        Promise<string> innerPromise = createPromise("result");
        string value = await innerPromise;
        print("2. After await: " + value);
        return value;
    } finally {
        print("3. Finally block runs before return");
    }
}

// Helper function to create a resolved promise
async function createPromise(string value): Promise<string> {
    return value;
}

import "../../lib/exceptions/Exception.mt";

// Run tests
print("=== Test 1: Finally before return ===");
Promise<string> p1 = finallyBeforeReturn();
string result1 = await p1;
print("Returned: " + result1);
print("");

print("=== Test 2: Finally overrides return ===");
Promise<string> p2 = finallyOverridesReturn();
string result2 = await p2;
print("Returned: " + result2 + " (should be 'second')");
print("");

print("=== Test 3: Finally after catch ===");
Promise<int> p3 = finallyAfterCatch();
int result3 = await p3;
print("Returned: " + toString(result3));
print("");

print("=== Test 4: Finally with await ===");
Promise<string> p4 = finallyWithAwait();
string result4 = await p4;
print("Returned: " + result4);
print("");

print("All finally-return-order tests completed");
