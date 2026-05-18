// Two instance methods with identical parameter signatures - rejected at class registration
// (overloading by NAME is allowed in mType; overloading by IDENTICAL SIGNATURE is not)
class Calculator {
    int compute(int a, int b) {
        return a + b;
    }

    // ERROR: Duplicate method signature: 'compute(int, int)'
    int compute(int x, int y) {
        return x * y;
    }
}

@Script
function main() {
    Calculator calc = new Calculator();
    print(calc.compute(1, 2));
}
