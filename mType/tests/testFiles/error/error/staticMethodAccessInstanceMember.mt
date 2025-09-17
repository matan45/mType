// Test error: accessing this keyword from static method
class TestClass {
    int instanceField = 10;

    static function staticMethod(): void {
        // This should cause an error: Cannot use 'this' in static method context
        TestClass obj = this;
        print(obj.instanceField);
    }
}

TestClass::staticMethod();