// Test: Abstract method implemented with wrong signature (parameter count)
// Expected: Error - signatures don't match

abstract class Calculator {
    abstract function compute(int x, int y): int;
}

class BadCalculator extends Calculator {
    // Wrong signature: only 1 parameter instead of 2
    function compute(int x): int {
        return x * 2;
    }
}

// This should fail because compute(int) doesn't match compute(int, int)
BadCalculator calc = new BadCalculator();
