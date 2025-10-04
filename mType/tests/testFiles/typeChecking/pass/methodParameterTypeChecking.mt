class Calculator {
    public int base;

    constructor(int b) {
        base = b;
    }

    public function add(int x): int {
        return base + x;
    }

    public function concat(string s): string {
        return base + s;
    }

    public static function multiply(int a, int b): int {
        return a * b;
    }
}

Calculator calc = new Calculator(10);

// Test correct method calls
int sum = calc.add(5);
string text = calc.concat(" units");
int product = Calculator::multiply(3, 4);

print(sum);
print(text);
print(product);