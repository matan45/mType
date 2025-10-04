// Test lambda lifecycle with scope boundaries
// Tests lambda behavior when crossing scope boundaries

interface Calculator {
    function calculate(int a, int b): int;
}

interface CallbackManager {
    function executeCallback(Calculator calc, int x, int y): int;
}

class SimpleCallbackManager implements CallbackManager {
    public function executeCallback(Calculator calc, int x, int y): int {
        return calc.calculate(x, y);
    }
}

function createAdder(): Calculator {
    // Lambda created in function scope and returned
    Calculator adder = (a, b) -> a + b;
    return adder;
}

function createMultiplier(): Calculator {
    // Another lambda crossing scope boundary
    return (a, b) -> a * b;
}

function createComplexCalculator(int factor): Calculator {
    // Lambda with captured context crossing scope
    return (a, b) -> (a + b) * factor;
}

function testScopeTransition(): Calculator {
    if (true) {
        // Lambda created in nested scope
        Calculator calculator = (a, b) -> {
            if (a > b) {
                return a - b;
            } else {
                return b - a;
            }
        };
        return calculator;
    }
    // This should never execute
    return (a, b) -> 0;
}

function main(): void {
    print("Testing lambda lifecycle across scopes...");

    SimpleCallbackManager manager = new SimpleCallbackManager();

    // Test 1: Lambda returned from function
    print("=== Test 1: Lambda from function scope ===");
    Calculator adder = createAdder();
    int result1 = manager.executeCallback(adder, 10, 20);
    print("Addition result: " + result1);

    // Test 2: Inline lambda return
    print("=== Test 2: Inline lambda return ===");
    Calculator multiplier = createMultiplier();
    int result2 = manager.executeCallback(multiplier, 6, 7);
    print("Multiplication result: " + result2);

    // Test 3: Lambda with capture
    print("=== Test 3: Lambda with captured context ===");
    Calculator complex1 = createComplexCalculator(2);
    Calculator complex2 = createComplexCalculator(3);

    int result3a = manager.executeCallback(complex1, 4, 5);
    int result3b = manager.executeCallback(complex2, 4, 5);
    print("Complex result 1 (factor 2): " + result3a);
    print("Complex result 2 (factor 3): " + result3b);

    // Test 4: Lambda from nested scope
    print("=== Test 4: Lambda from nested scope ===");
    Calculator differ = testScopeTransition();
    int result4a = manager.executeCallback(differ, 15, 10);
    int result4b = manager.executeCallback(differ, 8, 12);
    print("Difference result 1: " + result4a);
    print("Difference result 2: " + result4b);

    // Test 5: Multiple calls to same lambda
    print("=== Test 5: Repeated lambda usage ===");
    Calculator persistent = (a, b) -> a * a + b * b;
    for (int i = 1; i <= 3; i++) {
        int result = manager.executeCallback(persistent, i, i + 1);
        print("Squares sum for " + i + ": " + result);
    }

    print("Scope lifecycle test completed!");
}

main();