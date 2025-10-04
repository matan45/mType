// Test: Loop Scoping - Pass Cases
// Loop variables should be properly scoped

class TestClass {
    public function testForLoop(): void {
        print("=== For Loop Test ===");
        int outerVar = 100;
        for (int i = 0; i < 3; i = i + 1) {
            int loopVar = i * 10;
            print("for loop: i = " + i + ", loopVar = " + loopVar + ", outerVar = " + outerVar);
        }
        print("after for loop: outerVar = " + outerVar);
    }

    public function testWhileLoop(): void {
        print("=== While Loop Test ===");
        int outerVar = 200;
        int counter = 0;
        while (counter < 3) {
            int loopVar = counter * 20;
            print("while loop: counter = " + counter + ", loopVar = " + loopVar + ", outerVar = " + outerVar);
            counter = counter + 1;
        }
        print("after while loop: outerVar = " + outerVar + ", counter = " + counter);
    }

    public function testDoWhileLoop(): void {
        print("=== Do-While Loop Test ===");
        int outerVar = 300;
        int counter = 0;
        do {
            int loopVar = counter * 30;
            print("do-while loop: counter = " + counter + ", loopVar = " + loopVar + ", outerVar = " + outerVar);
            counter = counter + 1;
        } while (counter < 3);
        print("after do-while loop: outerVar = " + outerVar + ", counter = " + counter);
    }

    public function testNestedLoops(): void {
        print("=== Nested Loop Test ===");
        for (int i = 0; i < 2; i = i + 1) {
            int outerLoopVar = i * 100;
            print("outer loop: i = " + i + ", outerLoopVar = " + outerLoopVar);
            for (int j = 0; j < 2; j = j + 1) {
                int innerLoopVar = j * 10;
                print("  inner loop: j = " + j + ", innerLoopVar = " + innerLoopVar + ", outerLoopVar = " + outerLoopVar);
            }
        }
    }
}

TestClass obj = new TestClass();
obj.testForLoop();
obj.testWhileLoop();
obj.testDoWhileLoop();
obj.testNestedLoops();