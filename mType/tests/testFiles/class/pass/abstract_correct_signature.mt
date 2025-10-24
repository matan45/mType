// Test: Abstract method implemented with correct signature
// Expected: Success

abstract class Calculator {
    abstract function compute(int x, int y): int;
}

class AddCalculator extends Calculator {
    // Correct signature: 2 parameters matching abstract method
    public function compute(int x, int y): int {
        return x + y;
    }
}

// This should succeed
AddCalculator calc = new AddCalculator();
int result = calc.compute(5, 3);
print(result);  // Should print 8
