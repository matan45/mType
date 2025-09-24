// Test: Multiple naming convention errors in one file
// Expected Errors: Multiple function/method name violations

class Calculator {
    static function Add(int a, int b): int {
        return a + b;
    }

    function Multiply(int x, int y): int {
        return x * y;
    }
}

function Calculate(): int {
    return Calculator.Add(5, 3);
}

function main(): void {
    Calculator calc = new Calculator();
    int result1 = calc.Multiply(4, 5);
    int result2 = Calculate();
    print(result1);
    print(result2);
}

main();