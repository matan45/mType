import "../../lib/primitives/String.mt";
import "../../lib/primitives/Int.mt";

class MixedClass {
    // Instance fields
    String instanceField;

    // Constructor
    constructor(String field) {
        instanceField = field;
    }

    // Instance generic method
    function instanceGeneric(T value): T {
        print("Instance generic: " + value);
        return value;
    }

    // Static generic method
    static function <T> staticGeneric(T value): T {
        print("Static generic: " + value);
        return value;
    }

    // Regular instance method
    function regularInstance(): void {
        print("Regular instance method");
    }

    // Regular static method
    static function regularStatic(): void {
        print("Regular static method");
    }
}

// Test mixed usage
MixedClass obj = new MixedClass(new String("test"));

// Instance methods
obj.regularInstance();
String instanceResult = obj.instanceGeneric(new String("instance"));
print("Instance result: " + instanceResult);

// Static methods
MixedClass::regularStatic();
String staticResult = MixedClass::staticGeneric<String>(new String("static"));
print("Static result: " + staticResult);

Int staticIntResult = MixedClass::staticGeneric<Int>(new Int(123));
print("Static int result: " + staticIntResult);

print("Mixed static generic method tests passed");