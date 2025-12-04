// Test: Sentinel value edge cases for uninitialized slots
// Expected: Pass - uninitialized variables get proper type defaults

class Object {}

interface ObjSupplier {
    function get(): Object;
}

class Container {
    // Uninitialized fields should get type-appropriate defaults
    public int intField;
    public float floatField;
    public bool boolField;
    public string stringField;
    public Object objectField;

    public function showFields(): void {
        print("intField: " + this.intField);
        print("floatField: " + this.floatField);
        print("boolField: " + this.boolField);
        print("stringField: [" + this.stringField + "]");
        if (this.objectField == null) {
            print("objectField: null");
        } else {
            print("objectField: <object>");
        }
    }
}

interface Supplier {
    function get(): int;
}

function main(): void {
    print("=== Uninitialized Slot Edge Cases Test ===");

    // Test 1: Uninitialized class fields
    print("Test 1: Uninitialized class fields");
    Container c = new Container();
    c.showFields();
    if (c.intField == 0 && c.boolField == false && c.objectField == null) {
        print("Test 1 PASS: Fields have correct defaults");
    } else {
        print("Test 1 FAIL: Unexpected default values");
    }

    // Test 2: Local variables in sequential lambdas
    // This tests that lambda creation doesn't pollute slots for later lambdas
    print("Test 2: Sequential lambda creation");
    int a = 10;
    Supplier lambda1 = () -> a;

    int b = 20;
    Supplier lambda2 = () -> b;

    int c2 = 30;
    Supplier lambda3 = () -> c2;

    if (lambda1.get() == 10 && lambda2.get() == 20 && lambda3.get() == 30) {
        print("Test 2 PASS: Sequential lambdas capture correctly");
    } else {
        print("Test 2 FAIL: Lambda capture interference");
    }

    // Test 3: Lambda defined before captured variable is assigned
    print("Test 3: Forward reference in lambda");
    int laterVar = 0;
    Supplier getLater = () -> laterVar;
    laterVar = 42;
    if (getLater.get() == 42) {
        print("Test 3 PASS: Forward reference resolved correctly");
    } else {
        print("Test 3 FAIL: Expected 42, got " + getLater.get());
    }

    // Test 4: Multiple variables, some null some not
    print("Test 4: Mixed null and non-null captures");
    Object obj1 = null;
    Object obj2 = new Object();
    Object obj3 = null;

    ObjSupplier getObj1 = () -> obj1;
    ObjSupplier getObj2 = () -> obj2;
    ObjSupplier getObj3 = () -> obj3;

    if (getObj1.get() == null && getObj2.get() != null && getObj3.get() == null) {
        print("Test 4 PASS: Mixed captures work correctly");
    } else {
        print("Test 4 FAIL: Mixed capture error");
    }

    // Test 5: Nested function with local slots
    print("Test 5: Nested scope variables");
    int outer = 100;
    if (true) {
        int inner = 200;
        Supplier getInner = () -> inner + outer;
        if (getInner.get() == 300) {
            print("Test 5 PASS: Nested scope capture works");
        } else {
            print("Test 5 FAIL: Expected 300");
        }
    }

    print("=== Uninitialized Slot Edge Cases Test Complete ===");
}

main();
