// Test enhanced argument count mismatch exception with location info
class Calculator {
    function add(x: int, y: int): int {
        return x + y;
    }
}

Calculator calc = new Calculator();
int result = calc.add(5);  // Line 9: Should show argument count mismatch error with location
print("This should not execute");