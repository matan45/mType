class Calculator {
    function add(int x): int {
        return x + 1;
    }
}

Calculator calc = new Calculator();
calc.add("five");  // Should fail: string passed where int expected