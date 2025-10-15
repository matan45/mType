// Test file for unused method elimination

class Calculator {
    private int value;

    constructor(int initialValue) {
        value = initialValue;
    }

    // Used method
    public function add(int x): int {
        return value + x;
    }

    // UNUSED method - should be removed
    public function subtract(int x): int {
        return value - x;
    }

    // UNUSED method - should be removed
    public function multiply(int x): int {
        return value * x;
    }

    // UNUSED method - should be removed
    private function divide(int x): int {
        if (x == 0) {
            print("Cannot divide by zero");
            return 0;
        }
        return value / x;
    }

    // Used method (special method - always kept)
    public function toString(): string {
        return "Calculator(value=" + value + ")";
    }
}

class UnusedHelper {
    // This entire class is unused and should be removed
    public function helpMe(): void {
        print("I'm helping!");
    }

    private function privateHelper(): int {
        return 42;
    }
}

// This function is used
function testCalculator(): void {
    Calculator calc = new Calculator(10);
    int result = calc.add(5);
    print("Result: " + result);
    print(calc.toString());
}

// This function is UNUSED and should be removed
function unusedFunction(): void {
    print("I'm never called");
}


testCalculator();