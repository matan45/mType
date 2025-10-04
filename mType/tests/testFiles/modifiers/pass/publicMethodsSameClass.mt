// Test: Public methods accessible within same class
class Calculator {
    public function add(int a, int b): int {
        return a + b;
    }

    public function addThree(int a, int b, int c): int {
        return add(a, add(b, c));  // Calling public method from same class
    }
}

Calculator calc = new Calculator();
print(calc.addThree(1, 2, 3));  // Expected: 6
