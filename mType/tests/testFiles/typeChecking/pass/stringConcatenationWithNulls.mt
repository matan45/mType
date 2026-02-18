// Test string concatenation with null objects
// This tests various scenarios where null objects are involved in string concatenation

// Define a test class for object concatenation tests
class TestObject {
    public string name;
    public int value;

    constructor(string n, int v) {
        name = n;
        value = v;
    }

    public function toString(): string {
        return name + ":" + value;
    }
}

// Test 1: Direct null concatenation with strings
print("Testing direct null concatenation:");

TestObject? nullObj = null;
string result1 = "Hello " + nullObj;
print("\"Hello \" + toString(null) = " + result1);

string result2 = nullObj + " World";
print("toString(null) + \" World\" = " + result2);

// Test 2: Null object in complex string expressions
string prefix = "Object: ";
string suffix = " (end)";
string result3 = prefix + nullObj + suffix;
print("Complex null concatenation: " + result3);

// Test 3: Null object concatenation in conditional expressions
bool useNull = true;
TestObject obj = useNull ? null : new TestObject("test", 42);
string result4 = "Selected object: " + obj;
print("Conditional null: " + result4);

// Test 4: Null object in ternary expression concatenation
string result5 = "Value: " + (obj != null ? obj.toString() : "null");
print("Ternary null check: " + result5);

// Test 5: Multiple null objects in concatenation
TestObject? nullObj2 = null;
string result6 = nullObj + " and " + nullObj2;
print("Multiple nulls: " + result6);

// Test 6: Null object concatenation with function calls
function getNullObject(): TestObject {
    return null;
}

function getTestString(TestObject obj): string {
    return "Function result: " + obj;
}

string result7 = getTestString(getNullObject());
print("Function chain with null: " + result7);

// Test 7: Null object concatenation in loops
print("Null concatenation in loops:");
TestObject? objects = null;
TestObject object1 = new TestObject("first", 1);
TestObject? object2 = null;
TestObject object3 = new TestObject("third", 3);

for (int i = 0; i < 3; i++) {
if (i == 0) {
        objects = object1;
    } else if (i == 1) {
        objects = object2;
    } else {
        objects = object3;
    }
    string loopResult = "Item " + i + ": " + objects;
    print(loopResult);
}

// Test 8: Null object concatenation with assignment operations
string accumulated = "";
accumulated += "Start ";
accumulated += nullObj;
accumulated += " Middle ";
accumulated += nullObj2;
accumulated += " End";
print("Accumulated string: " + accumulated);

// Test 9: Null object concatenation in class methods
class StringProcessor {
    public function process(TestObject obj): string {
        return "Processed: " + obj;
    }

    public function combineObjects(TestObject obj1, TestObject obj2): string {
        return obj1 + " combined with " + obj2;
    }
}

StringProcessor processor = new StringProcessor();
string result8 = processor.process(null);
print("Method with null: " + result8);

string result9 = processor.combineObjects(null, new TestObject("valid", 100));
print("Method combining null and valid: " + result9);

// Test 10: Nested null object concatenation
TestObject validObj = new TestObject("nested", 50);
string result10 = "Outer: " + ("Inner: " + nullObj + " with " + validObj.toString());
print("Nested concatenation: " + result10);

// Test 11: Null object concatenation with different data types
int number = 42;
bool flag = true;
float decimal = 3.14;

string mixedResult = "Mixed: " + number + ", " + nullObj + ", " + flag + ", " + decimal;
print("Mixed types with null: " + mixedResult);

// Test 12: Null object concatenation error handling (should not crash)
TestObject? potentialNull = null;
string safeResult = potentialNull != null ?
                   "Object exists: " + potentialNull.toString() :
                   "Object is null: " + potentialNull;
print("Safe null handling: " + safeResult);

// Test 13: Null object concatenation in switch-like logic
function processObjectType(TestObject obj): string {
    if (obj == null) {
        return "Type: null object - " + obj;
    } else {
        return "Type: valid object - " + obj.toString();
    }
}

print("Switch-like null processing:");
print(processObjectType(null));
print(processObjectType(new TestObject("switch", 123)));

// Test 14: Null object concatenation with static methods
class StaticStringUtils {
    public static function formatObject(TestObject obj): string {
        return "[" + obj + "]";
    }

    public static function combineWithPrefix(string prefix, TestObject obj): string {
        return prefix + obj;
    }
}

string staticResult1 = StaticStringUtils::formatObject(null);
print("Static method with null: " + staticResult1);

string staticResult2 = StaticStringUtils::combineWithPrefix("PREFIX:", null);
print("Static method with prefix and null: " + staticResult2);

// Test 15: Null object concatenation in exception scenarios (edge cases)
TestObject? nullArray = null;
string arrayResult = "Array: " + nullArray;
print("Null array concatenation: " + arrayResult);

// Test 16: Complex nested null scenarios
class Container {
    public TestObject inner;

    constructor(TestObject obj) {
        inner = obj;
    }

    public function describe(): string {
        return "Container holding: " + inner;
    }
}

Container containerWithNull = new Container(null);
string containerResult = "Container test: " + containerWithNull.describe();
print("Container with null inner: " + containerResult);

Container? nullContainer = null;
string nullContainerResult = "Null container: " + nullContainer;
print("Null container object: " + nullContainerResult);

print("String concatenation with null objects test completed successfully");