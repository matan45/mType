// Test: Lambda capturing explicitly null variables
// Expected: Pass - null values should be properly captured and accessible

interface Supplier {
    function get(): Object?;
}

interface Consumer {
    function accept(Object o): void;
}

class Container {
    public Object? value;

    constructor(Object? v) {
        this.value = v;
    }
}

class Object {}

function main(): void {
    print("=== Lambda Capture Null Test ===");

    // Test 1: Capture explicitly null variable
    Object? nullObj = null;
    Supplier getNullObj = () -> nullObj;
    Object? result1 = getNullObj.get();
    if (result1 == null) {
        print("Test 1 PASS: Captured null variable correctly");
    } else {
        print("Test 1 FAIL: Expected null");
    }

    // Test 2: Capture null and verify it stays null
    Object? anotherNull = null;
    Supplier getAnother = () -> anotherNull;
    if (getAnother.get() == null) {
        print("Test 2 PASS: Null capture persists");
    } else {
        print("Test 2 FAIL: Expected null to persist");
    }

    // Test 3: Capture null in a container context
    Container container = new Container(null);
    Supplier getContainerValue = () -> container.value;
    if (getContainerValue.get() == null) {
        print("Test 3 PASS: Captured null field through object");
    } else {
        print("Test 3 FAIL: Expected null field");
    }

    // Test 4: Assign non-null, then null, then capture
    Object? changingObj = new Object();
    changingObj = null;
    Supplier getChanging = () -> changingObj;
    if (getChanging.get() == null) {
        print("Test 4 PASS: Captured reassigned null");
    } else {
        print("Test 4 FAIL: Expected null after reassignment");
    }

    // Test 5: Multiple lambdas capturing same null
    Object? sharedNull = null;
    Supplier lambda1 = () -> sharedNull;
    Supplier lambda2 = () -> sharedNull;
    if (lambda1.get() == null && lambda2.get() == null) {
        print("Test 5 PASS: Multiple lambdas captured same null");
    } else {
        print("Test 5 FAIL: Expected both to be null");
    }

    print("=== Lambda Capture Null Test Complete ===");
}

main();
