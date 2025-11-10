import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test nested try-catch in async functions
function async nestedTryCatch(): Promise<String> {
    try {
        print("Outer try");
        try {
            print("Inner try");
            Exception e = new Exception("Inner exception");
            throw e;
        } catch (Exception e) {
            print("Inner catch: " + e.getMessage());
            return new String("inner caught");
        } finally {
            print("Inner finally");
        }
    } catch (Exception e) {
        print("Outer catch: " + e.getMessage());
        return new String("outer caught");
    } finally {
        print("Outer finally");
    }
}

// Test re-throwing in catch block
function async rethrowInCatch(): Promise<Int> {
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
        return new Int(999);
    }
}

// Test exception in finally block
function async exceptionInFinally(): Promise<String> {
    try {
        try {
            print("Try block");
            return new String("from try");
        } finally {
            print("Finally throws exception");
            Exception e = new Exception("Finally error");
            throw e;  // Exception in finally overrides return
        }
    } catch (Exception e) {
        print("Caught finally exception: " + e.getMessage());
        return new String("caught finally exception");
    }
}

// Test multiple await with try-catch
function async multipleAwaitWithTryCatch(): Promise<Int> {
    int total = 0;

    try {
        Promise<Int> p1 = getValue(10);
        Int val1 = await p1;
        print("Got value 1: " + val1.toString());
        total = total + val1.getValue();

        Promise<Int> p2 = getValue(20);
        Int val2 = await p2;
        print("Got value 2: " + val2.toString());
        total = total + val2.getValue();

        return new Int(total);
    } catch (Exception e) {
        print("Error in await: " + e.getMessage());
        return new Int(-1);
    } finally {
        print("Finally: total = " + total);
    }
}

function async getValue(int value): Promise<Int> {
    return new Int(value);
}

function async main(): Promise<void> {
// Run tests
print("=== Test 1: Nested try-catch ===");
Promise<String> p1 = nestedTryCatch();
String result1 = await p1;
print("Result: " + result1.toString());
print("");

print("=== Test 2: Re-throw in catch ===");
Promise<Int> p2 = rethrowInCatch();
Int result2 = await p2;
print("Result: " + result2.toString());
print("");

print("=== Test 3: Exception in finally ===");
Promise<String> p3 = exceptionInFinally();
String result3 = await p3;
print("Result: " + result3.toString());
print("");

print("=== Test 4: Multiple await with try-catch ===");
Promise<Int> p4 = multipleAwaitWithTryCatch();
Int result4 = await p4;
print("Result: " + result4.toString());
print("");

print("All nested try-catch tests completed");
return null;
}

main();
