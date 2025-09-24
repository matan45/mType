// Test: Conditional Scoping - Pass Cases
// Variables in if/else and switch blocks should be properly scoped

class TestClass {
    function testIfElseScoping(): void {
        print("=== If-Else Scoping Test ===");
        int outerVar = 500;
        bool condition = true;

        if (condition) {
            int ifVar = 111;
            print("if block: ifVar = " + ifVar + ", outerVar = " + outerVar);
        } else {
            int elseVar = 222;
            print("else block: elseVar = " + elseVar + ", outerVar = " + outerVar);
        }

        print("after if-else: outerVar = " + outerVar);
    }

    function testNestedIf(): void {
        print("=== Nested If Test ===");
        int outerVar = 600;
        bool cond1 = true;
        bool cond2 = true;

        if (cond1) {
            int level1Var = 111;
            print("level1: level1Var = " + level1Var + ", outerVar = " + outerVar);

            if (cond2) {
                int level2Var = 222;
                print("level2: level2Var = " + level2Var + ", level1Var = " + level1Var + ", outerVar = " + outerVar);
            }

            print("back to level1: level1Var = " + level1Var + ", outerVar = " + outerVar);
        }
    }

    function testSwitchScoping(): void {
        print("=== Switch Scoping Test ===");
        int outerVar = 700;
        int value = 1;

        switch (value) {
            case 1: {
                int case1Var = 111;
                print("case 1: case1Var = " + case1Var + ", outerVar = " + outerVar);
                break;
            }
            case 2: {
                int case2Var = 222;
                print("case 2: case2Var = " + case2Var + ", outerVar = " + outerVar);
                break;
            }
            default: {
                int defaultVar = 333;
                print("default: defaultVar = " + defaultVar + ", outerVar = " + outerVar);
                break;
            }
        }

        print("after switch: outerVar = " + outerVar);
    }
}

TestClass obj = new TestClass();
obj.testIfElseScoping();
obj.testNestedIf();
obj.testSwitchScoping();