// Test: Abstract classes with constant folding optimization
// Verifies constant folding works correctly with abstract methods

abstract class Calculator {
    protected int precision;

    public constructor(int p) {
        this.precision = p;
    }

    abstract function calculate(int a, int b): int;

    // Method with constant expressions that should be folded
    public function getPrecisionSquared(): int {
        return 2 * 2;  // Should be folded to 4
    }

    public function isValidPrecision(): bool {
        return true && true;  // Should be folded to true
    }

    public function getMaxValue(): int {
        if (1 + 1 == 2) {  // Should be folded to if(true)
            return 100;
        } else {
            return 0;  // Dead branch, should be removed
        }
    }
}

class Adder extends Calculator {
    public constructor() : super(2) {
    }

    public function calculate(int a, int b): int {
        int result = a + b;
        // Constant folding in implementation
        int bonus = 5 + 5;  // Should be folded to 10
        return result + bonus;
    }
}

class Multiplier extends Calculator {
    public constructor() : super(4) {
    }

    public function calculate(int a, int b): int {
        // Constant folding with multiplication
        int factor = 3 * 2;  // Should be folded to 6
        return a * b * factor;
    }
}

// Test execution
Adder adder = new Adder();
print("Add result: " + adder.calculate(10, 20));
print("Precision squared: " + adder.getPrecisionSquared());
print("Valid: " + adder.isValidPrecision());
print("Max: " + adder.getMaxValue());

Multiplier mult = new Multiplier();
print("Multiply result: " + mult.calculate(3, 4));

print("Test Complete!");
