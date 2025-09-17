// Test error: accessing 'this' from static method
class TestClass {
    int instanceField = 10;

    static function staticMethod(): void {
        // This should cause an error: Cannot use 'this' in static method context
        print(this.instanceField);
    }
}

TestClass::staticMethod();