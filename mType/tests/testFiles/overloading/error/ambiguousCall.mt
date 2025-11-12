// Ambiguous overload resolution - should fail at compile time
class Calculator {
    int compute(int a, float b) {
        return a + b;
    }

    int compute(float a, int b) {
        return a + b;
    }
}

@Script
function main() {
    Calculator calc = new Calculator();

    // This should be ambiguous - both overloads are equally specific
    // compute(int, float) matches with (int, float->float) conversion
    // compute(float, int) matches with (int->float, int) conversion
    int result = calc.compute(10, 20);  // ERROR: Ambiguous call
}
