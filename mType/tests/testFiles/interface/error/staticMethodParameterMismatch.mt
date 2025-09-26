// Test interface parameter type mismatch error with static methods
interface Function {
    function apply(int x) : int;
}

class Calculator {
    int value;
    constructor(int v) { value = v; }
    function getValue() : int { return value; }
    // Note: Calculator does NOT implement Function interface
}

class MathProcessor {
    static function processWithFunction(Function f, int input) : int {
        return f.apply(input);  // This should fail during parameter validation
    }

    static function doubleProcess(Function f1, Function f2, int value) : int {
        int result1 = f1.apply(value);
        return f2.apply(result1);
    }
}

print("Testing static method parameter mismatch");
Calculator calc = new Calculator(42);

// This should fail with parameter type mismatch error
// Calculator doesn't implement Function interface
int result = MathProcessor::processWithFunction(calc, 10);
print("Result: " + result);