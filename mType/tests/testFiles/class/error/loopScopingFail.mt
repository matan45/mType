// Test: Loop Scoping - Fail Case
// Loop variables should NOT be accessible outside the loop

class TestClass {
    void testForLoopScoping(): void {
        for (int i = 0; i < 3; i = i + 1) {
            int loopVar = i * 10;
            print("inside loop: i = " + i + ", loopVar = " + loopVar);
        }
        // These should fail - loop variables are out of scope
        print("trying to access i outside loop: " + i); // Should error
        print("trying to access loopVar outside loop: " + loopVar); // Should error
    }
}

TestClass obj = new TestClass();
obj.testForLoopScoping();