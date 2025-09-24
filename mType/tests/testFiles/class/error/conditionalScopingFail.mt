// Test: Conditional Scoping - Fail Case
// Variables in if blocks should NOT be accessible outside the block

class TestClass {
    void testIfScopingFail(): void {
        bool condition = true;

        if (condition) {
            int ifVar = 111;
            print("inside if: ifVar = " + ifVar);
        }

        // This should fail - ifVar is out of scope
        print("trying to access ifVar outside if: " + ifVar); // Should error
    }
}

TestClass obj = new TestClass();
obj.testIfScopingFail();