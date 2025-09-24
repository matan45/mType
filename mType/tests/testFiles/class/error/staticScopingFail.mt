// Test: Static Method Scoping - Fail Case
// Static methods should NOT access instance fields

class TestClass {
    int instanceField = 777;
    static int staticField = 555;

    static void staticMethodBad(): void {
        print("Trying to access instanceField: " + instanceField); // Should error
    }
}

TestClass.staticMethodBad();