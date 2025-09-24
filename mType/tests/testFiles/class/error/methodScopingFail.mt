// Test: Method Scoping - Fail Case
// Methods should NOT access local variables from other methods

class TestClass {
    void methodA(): void {
        int localVar = 42;
        print("methodA: localVar = " + localVar);
        methodB(); // This should fail when methodB tries to access localVar
    }

    void methodB(): void {
        print("methodB: trying to access localVar = " + localVar); // Should error
    }
}

TestClass obj = new TestClass();
obj.methodA();