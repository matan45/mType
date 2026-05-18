// Ambiguous overload resolution - should fail at compile time
class Calculator {
    public function compute(int a, float b): int {
        return 0;
    }

    public function compute(float a, int b): int {
        return 0;
    }
}

function main(): void {
    Calculator calc = new Calculator();

    // This should be ambiguous - both overloads are equally specific
    // compute(int, float) matches with (int, float->float) conversion
    // compute(float, int) matches with (int->float, int) conversion
    int result = calc.compute(10, 20);  // ERROR: Ambiguous call
}
