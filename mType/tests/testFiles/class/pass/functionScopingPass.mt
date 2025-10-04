// Test: Function Scoping - Pass Cases
// Functions should have proper lexical scoping (access global, not other functions)

int globalVar = 999;

function helperFunction(): int {
    int localVar = 123;
    print("helperFunction: localVar = " + localVar);
    print("helperFunction: globalVar = " + globalVar);
    return localVar;
}

function mainFunction(): void {
    int localVar = 456; // Different from helperFunction's localVar
    print("mainFunction: localVar = " + localVar);
    print("mainFunction: globalVar = " + globalVar);

    // Can call other functions
    int result = helperFunction();
    print("mainFunction: result from helper = " + result);
    print("mainFunction: localVar still = " + localVar); // Should be unchanged
}

function nestedScopeFunction(): void {
    int outerVar = 111;
    print("outerVar = " + outerVar);

    if (true) {
        int innerVar = 222;
        print("innerVar = " + innerVar + ", outerVar = " + outerVar);

        for (int i = 0; i < 2; i = i + 1) {
            int loopVar = i * 10;
            print("loopVar = " + loopVar + ", innerVar = " + innerVar + ", outerVar = " + outerVar);
        }
    }

    print("back to function scope: outerVar = " + outerVar);
}

class TestClass {
    public function callFunctions(): void {
        print("=== Calling Functions from Class ===");
        mainFunction();
        nestedScopeFunction();
    }
}

print("=== Direct Function Calls ===");
mainFunction();
nestedScopeFunction();

print("=== Function Calls from Class ===");
TestClass obj = new TestClass();
obj.callFunctions();