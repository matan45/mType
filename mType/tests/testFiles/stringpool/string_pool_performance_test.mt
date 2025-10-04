// StringPool performance and memory efficiency test
// Tests repeated string usage to verify pool effectiveness

function testRepeatedStrings(): void {
    print("Testing repeated string usage");

    // Common identifiers that should be heavily pooled
    for (int i = 0; i < 10; i = i + 1) {
        string value = "value";
        string result = "result";
        string data = "data";
        string name = "name";
        string type = "type";

        print("Iteration: " + i);
        print("Value: " + value);
        print("Result: " + result);
        print("Data: " + data);
    }
}

function testStringOperations(): void {
    print("Testing string operations with pooled strings");

    string base = "base";
    string suffix = "suffix";

    for (int i = 0; i < 5; i = i + 1) {
        string combined = base + "_" + suffix;
        print("Combined " + i + ": " + combined);

        if (base == "base") {
            print("String equality check passed");
        }
    }
}

 class TestClass {
        string fieldName;
        string fieldValue;

        public function init(string name, string value) {
            this.fieldName = name;
            this.fieldValue = value;
        }

        public function getInfo() {
            return "TestType:" + this.fieldName + "=" + this.fieldValue;
        }
    }

function testClassFieldStrings(): void {
    print("Testing string usage in class context");

    TestClass obj1 = new TestClass();
    obj1.init("username", "alice");
    print(obj1.getInfo());

    TestClass obj2 = new TestClass();
    obj2.init("username", "bob");  // "username" reused from pool
    print(obj2.getInfo());
}

function testStringComparisonPerformance(): void {
    print("Testing string comparison performance");

    string str1 = "performance_test_string";
    string str2 = "performance_test_string";  // Should reuse from pool
    string str3 = "different_string";

    for (int i = 0; i < 100; i = i + 1) {
        if (str1 == str2) {
            // Pool-optimized comparison should be very fast
            continue;
        }

        if (str1 != str3) {
            // Different strings comparison
            continue;
        }
    }

    print("String comparison performance test completed");
}

// Entry point
testRepeatedStrings();
testStringOperations();
testClassFieldStrings();
testStringComparisonPerformance();