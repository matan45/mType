// Test optimization with imports
import * from "MathLibrary.mt";

// Helper class with mixed usage
class Calculator {
    private int result;

    constructor() {
        result = 0;
    }

    // Used method
    public function calculate(int a, int b): int {
        result = UsedMath::add(a, b);
        return result;
    }

    // UNUSED method - should be removed
    public function reset(): void {
        result = 0;
    }

    // UNUSED method - should be removed
    private function doubleResult(): int {
        return result * 2;
    }

    // Special method - always kept
    public function toString(): string {
        return "Calculator(result=" + result + ")";
    }
}

// UNUSED class - should be removed
class UnusedCalculator {
    public function compute(): int {
        return 0;
    }
}

// UNUSED function - should be removed
function unusedHelper(): void {
    print("Never called");
}

// Main entry point
function main(): void {
    Calculator calc = new Calculator();
    int value = calc.calculate(10, 20);
    print("Result: " + value);
    print(calc.toString());
}

main();
