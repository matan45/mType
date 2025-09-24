// Test: Static Method and Field Scoping - Pass Cases

class TestClass {
    static int staticField = 555;
    int instanceField = 777;

    static function staticMethodA(): void {
        int localVar = 111;
        print("staticMethodA: localVar = " + localVar);
        print("staticMethodA: staticField = " + staticField);
        this::staticMethodB(); // Should work - calling another static method in same class
    }

    static function staticMethodB(): void {
        int localVar = 222; // Different localVar than staticMethodA
        print("staticMethodB: localVar = " + localVar);
        print("staticMethodB: staticField = " + staticField);
    }

    function instanceMethod(): void {
        int localVar = 333;
        print("instanceMethod: localVar = " + localVar);
        print("instanceMethod: instanceField = " + instanceField);
        print("instanceMethod: staticField = " + staticField); // Can access static
        TestClass::staticMethodA(); // Can call static method
    }
}

// Test static method calls
TestClass::staticMethodA();
TestClass::staticMethodB();

// Test instance method
TestClass obj = new TestClass();
obj.instanceMethod();