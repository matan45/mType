class Calculator {
    function getValue(): int {
        return 42;
    }
}

Calculator calc = new Calculator();
string wrongValue = calc.getValue();  // Should fail