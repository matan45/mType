// Test: Method Scoping - Pass Cases
// Variables in methods should be isolated from other methods

class TestClass {
    int classField = 100;

    function methodA(): int {
        int localVar = 42;
        print("methodA: localVar = " + localVar);
        print("methodA: classField = " + classField);
        return localVar;
    }

    function methodB(): int {
        int localVar = 99; // Different localVar than methodA
        print("methodB: localVar = " + localVar);
        print("methodB: classField = " + classField);
        return localVar;
    }

    function testNested(): void {
        int outerVar = 10;
        if (true) {
            int innerVar = 20;
            print("nested: outerVar = " + outerVar);
            print("nested: innerVar = " + innerVar);
        }
        print("after nested: outerVar = " + outerVar);
    }
}

TestClass obj = new TestClass();
int resultA = obj.methodA();
int resultB = obj.methodB();
obj.testNested();

print("Results: " + resultA + ", " + resultB);