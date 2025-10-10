import "../../lib/exceptions/Exception.mt";
import "../../lib/exceptions/RuntimeException.mt";

// Test nested try-catch in async functions
async function nestedTryCatch(): Promise<string> {
    try {
        print("Outer try");
        try {
            print("Inner try");
            Exception e = new Exception("Inner exception");
            throw e;
        } catch (Exception e) {
            print("Inner catch: " + e.getMessage());
            return "inner caught";
        } finally {
            print("Inner finally");
        }
    } catch (Exception e) {
        print("Outer catch: " + e.getMessage());
        return "outer caught";
    } finally {
        print("Outer finally");
    }
}

// Test re-throwing in catch block
async function rethrowInCatch(): Promise<int> {
    try {
        try {
            print("Inner try - throwing");
            RuntimeException e = new RuntimeException("Original error");
            throw e;
        } catch (RuntimeException e) {
            print("Inner catch - re-throwing");
            throw e;  // Re-throw to outer catch
        }
    } catch (RuntimeException e) {
        print("Outer catch: " + e.getMessage());
        return 999;
    }
}

// Test exception in finally block
async function exceptionInFinally(): Promise<string> {
    try {
        try {
            print("Try block");
            return "from try";
        } finally {
            print("Finally throws exception");
            Exception e = new Exception("Finally error");
            throw e;  // Exception in finally overrides return
        }
    } catch (Exception e) {
        print("Caught finally exception: " + e.getMessage());
        return "caught finally exception";
    }
}

// Test multiple await with try-catch
async function multipleAwaitWithTryCatch(): Promise<int> {
    int total = 0;

    try {
        Promise<int> p1 = getValue(10);
        int val1 = await p1;
        print("Got value 1: " + toString(val1));
        total = total + val1;

        Promise<int> p2 = getValue(20);
        int val2 = await p2;
        print("Got value 2: " + toString(val2));
        total = total + val2;

        return total;
    } catch (Exception e) {
        print("Error in await: " + e.getMessage());
        return -1;
    } finally {
        print("Finally: total = " + toString(total));
    }
}

async function getValue(int value): Promise<int> {
    return value;
}

// Run tests
print("=== Test 1: Nested try-catch ===");
Promise<string> p1 = nestedTryCatch();
string result1 = await p1;
print("Result: " + result1);
print("");

print("=== Test 2: Re-throw in catch ===");
Promise<int> p2 = rethrowInCatch();
int result2 = await p2;
print("Result: " + toString(result2));
print("");

print("=== Test 3: Exception in finally ===");
Promise<string> p3 = exceptionInFinally();
string result3 = await p3;
print("Result: " + result3);
print("");

print("=== Test 4: Multiple await with try-catch ===");
Promise<int> p4 = multipleAwaitWithTryCatch();
int result4 = await p4;
print("Result: " + toString(result4));
print("");

print("All nested try-catch tests completed");
