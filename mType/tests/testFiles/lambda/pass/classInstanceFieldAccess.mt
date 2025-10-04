// Test lambda accessing instance fields from class context
interface Function {
    function apply(int x) : int;
}

class Calculator {
    int multiplier;
    int offset;

    constructor(int mult, int off) {
        multiplier = mult;
        offset = off;
    }

    public function getMultiplierLambda() : Function {
        // Lambda accessing instance field directly
        return x -> x * multiplier;
    }

    public function getComplexLambda() : Function {
        // Lambda accessing multiple instance fields
        return x -> (x + offset) * multiplier;
    }

    public function getBlockLambda() : Function {
        // Block lambda with instance field access
        return x -> {
            int temp = x + offset;
            temp = temp * multiplier;
            return temp + offset;
        };
    }
}

print("Testing lambda instance field access");

Calculator calc = new Calculator(3, 5);

Function mult = calc.getMultiplierLambda();
print("Multiplier lambda (4): " + mult.apply(4));

Function complex = calc.getComplexLambda();
print("Complex lambda (2): " + complex.apply(2));

Function block = calc.getBlockLambda();
print("Block lambda (6): " + block.apply(6));

print("Lambda instance field access test completed");