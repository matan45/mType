// Test: Public methods accessible within same class
class Calculator {
    public int add(int a, int b) {
        return a + b;
    }

    public int addThree(int a, int b, int c) {
        return add(a, add(b, c));  // Calling public method from same class
    }
}

Calculator calc = new Calculator();
print(calc.addThree(1, 2, 3));  // Expected: 6
