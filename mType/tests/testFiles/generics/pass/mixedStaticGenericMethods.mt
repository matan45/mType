import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

class MixedClass {
    // Instance fields
    String instanceField;

    // Constructor
    public constructor(String field) {
        instanceField = field;
    }

    // Instance generic method
    public function instanceGeneric(T value): T {
        print("Instance generic: " + value);
        return value;
    }

    // Static generic method
    public static function <T> staticGeneric(T value): T {
        print("Static generic: " + value);
        return value;
    }

    // Regular instance method
    public function regularInstance(): void {
        print("Regular instance method");
    }

    // Regular static method
    public static function regularStatic(): void {
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