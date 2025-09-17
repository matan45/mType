// Test error: calling instance method from static method
class TestClass {
    int instanceField = 10;

    function instanceMethod(): void {
        print("Instance method called");
    }

    static function staticMethod(): void {
        TestClass obj = new TestClass();
        // This should cause an error: Cannot call instance method from static method context
        obj.instanceMethod();
    }
}

TestClass::staticMethod();