class Calculator {
    public int result;

    public function add(int a, int b): int {
        result = a + b;
        return result;
    }

    public function multiply(int a, int b): int {
        result = a * b;
        return result;
    }

    public function getResult(): int {
        return result;
    }
}

Calculator calc = new Calculator();
int sum = calc.add(5, 3);
print(sum); // 8

int product = calc.multiply(4, 7);
print(product); // 28

print(calc.getResult()); // 28