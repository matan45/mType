import "../../lib/primitives/String.mt";

class TestClass {
    // This is NOT a generic method
    static function regularMethod(String value): void {
        print("Regular method: " + value);
    }
}

// This should fail - providing type arguments to non-generic method
TestClass::regularMethod<String>(new String("error"));