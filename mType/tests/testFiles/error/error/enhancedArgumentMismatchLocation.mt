// Test enhanced argument count mismatch exception with location info
class Calculator {
    function add(int x, int y): int {
        return x + y;
    }
}

Calculator calc = new Calculator();
int result = calc.add(5);  // Line 9: Should show argument count mismatch error with location
print("This should not execute");