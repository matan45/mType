// Test: Static Method and Field Scoping - Pass Cases

class TestClass {
    public static int staticField = 555;
    public static int staticField2 = 555;
    public int instanceField = 777;

    public static function staticMethodA(): void {
        int localVar = 111;
		staticField2++;
        print("staticMethodA: localVar = " + localVar);
        print("staticMethodA: staticField = " + staticField);
        staticMethodB(); // Should work - calling another static method in same class
    }

    public static function staticMethodB(): void {
        int localVar = 222; // Different localVar than staticMethodA
        print("staticMethodB: localVar = " + localVar);
        print("staticMethodB: staticField = " + staticField);
    }

    public function instanceMethod(): void {
        int localVar = 333;
        print("instanceMethod: localVar = " + localVar);
        print("instanceMethod: instanceField = " + instanceField);
        print("instanceMethod: staticField = " + staticField); // Can access static
        staticMethodA(); // Can call static method
    }
}

// Test static method calls
TestClass::staticMethodA();
TestClass::staticMethodB();

// Test instance method
TestClass obj = new TestClass();
obj.instanceMethod();