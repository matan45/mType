// Basic Type Conversion Test
// Tests primitive type handling with improved TypeRegistry

function testPrimitiveTypes(): void {
    print("=== Testing Primitive Types ===");

    // Test basic type assignments
    int intValue = 42;
    float floatValue = 3.14;
    bool boolValue = true;
    string stringValue = "Hello TypeRegistry!";

    print("int: " + intValue);
    print("float: " + floatValue);
    print("bool: " + boolValue);
    print("string: " + stringValue);

    // Test type conversions
    float convertedInt = intValue; // int to float conversion
    print("int to float: " + convertedInt);
}

function testObjectTypes(): void {
    print("=== Testing Object Types ===");

    // Test with a simple class
    class TestClass {
         int value;

        constructor(int val) {
            this.value = val;
        }

        function toString(): string {
            return "TestClass(" + value + ")";
        }
    }

    TestClass obj = new TestClass(100);
    print("Custom object: " + obj.toString());
}

function testNullHandling(): void {
    print("=== Testing Null Handling ===");

    class NullTestClass {
        int data;
        constructor(int d) { this.data = d; }
    }

    NullTestClass nullObj = null;
    print("Null object created");

    if (nullObj == null) {
        print("Null check passed");
    }

    nullObj = new NullTestClass(50);
    print("Object assigned: " + nullObj.data);
}

function main(): void {
    print("Starting Type Conversion Basic Tests...");

    testPrimitiveTypes();
    testObjectTypes();
    testNullHandling();

    print("Basic type conversion tests completed!");
}

main();