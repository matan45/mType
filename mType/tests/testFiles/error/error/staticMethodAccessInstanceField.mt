// Test error: accessing instance field from static method
class TestClass {
    int instanceField = 10;
    static int staticField = 20;

    static function staticMethod(): void {
        // This should cause an error: Cannot access instance field from static method context
        print(instanceField);  // Direct access to instance field should fail
        print(staticField);    // This should work fine
    }
}

TestClass::staticMethod();