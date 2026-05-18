// Two instance methods with identical parameter signatures - rejected at class registration
// (overloading by NAME is allowed in mType; overloading by IDENTICAL SIGNATURE is not)
class Calculator {
    public function compute(int a, int b): int {
        return a + b;
    }

    // ERROR: Duplicate method signature: 'compute(int, int)'
    public function compute(int x, int y): int {
        return x * y;
    }
}

function main(): void {
    Calculator calc = new Calculator();
    print(calc.compute(1, 2));
}
