// Test: Distinguishing void returns from null returns
// Expected: Pass - void and null have different semantics

class Object {}

class TestClass {
    // Method that returns void (no return value)
    public function doNothing(): void {
        print("doNothing executed");
        // No return statement - this is void
    }

    // Method that explicitly returns null
    public function getNull(): Object {
        return null;
    }

    // Method that returns an actual object
    public function getObject(): Object {
        return new Object();
    }

    // Method that conditionally returns null or object
    public function getMaybe(bool returnNull): Object {
        if (returnNull) {
            return null;
        }
        return new Object();
    }
}

function voidFunction(): void {
    print("voidFunction executed");
    // Implicit void return
}

function nullFunction(): Object {
    return null;
}

function main(): void {
    print("=== Void vs Null Returns Test ===");

    TestClass t = new TestClass();

    // Test 1: Void method executes without returning a value
    print("Test 1: Calling void method");
    t.doNothing();
    print("Test 1 PASS: Void method completed");

    // Test 2: Null return is an actual value
    print("Test 2: Calling null-returning method");
    Object nullResult = t.getNull();
    if (nullResult == null) {
        print("Test 2 PASS: Got null as expected");
    } else {
        print("Test 2 FAIL: Expected null");
    }

    // Test 3: Object return is not null
    print("Test 3: Calling object-returning method");
    Object objResult = t.getObject();
    if (objResult != null) {
        print("Test 3 PASS: Got non-null object");
    } else {
        print("Test 3 FAIL: Expected non-null");
    }

    // Test 4: Conditional null vs object
    print("Test 4: Conditional returns");
    Object maybe1 = t.getMaybe(true);
    Object maybe2 = t.getMaybe(false);
    if (maybe1 == null && maybe2 != null) {
        print("Test 4 PASS: Conditional returns work correctly");
    } else {
        print("Test 4 FAIL: Unexpected conditional result");
    }

    // Test 5: Global void function
    print("Test 5: Global void function");
    voidFunction();
    print("Test 5 PASS: Global void function completed");

    // Test 6: Global null-returning function
    print("Test 6: Global null function");
    Object globalNull = nullFunction();
    if (globalNull == null) {
        print("Test 6 PASS: Global function returned null");
    } else {
        print("Test 6 FAIL: Expected null");
    }

    // Test 7: Null equality checks
    print("Test 7: Null equality");
    Object? a = null;
    Object? b = null;
    Object c = new Object();
    if (a == b && a == null && c != null && a != c) {
        print("Test 7 PASS: Null equality works correctly");
    } else {
        print("Test 7 FAIL: Null equality broken");
    }

    print("=== Void vs Null Returns Test Complete ===");
}

main();
