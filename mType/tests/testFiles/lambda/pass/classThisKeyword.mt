// Test lambda with explicit 'this' keyword access
interface Processor {
    function process(int value) : int;
}

interface BinaryProcessor {
    function process(int a, int b) : int;
}

class Calculator {
    int factor;
    int base;

    constructor(int f, int b) {
        factor = f;
        base = b;
    }

    public function getThisLambda() : Processor {
        // Lambda using explicit this.field syntax
        return x -> x * this.factor + this.base;
    }

    public function getMultipleThisLambda() : BinaryProcessor {
        // Lambda with multiple this accesses
        return (a, b) -> a * this.factor + b * this.base;
    }

    public function getThisBlockLambda() : Processor {
        // Block lambda with this keyword
        return x -> {
            int temp = this.factor * 2;
            temp = temp + x;
            return temp + this.base;
        };
    }

    public function getMixedAccessLambda() : Processor {
        // Mixed this and direct field access
        return x -> x + this.factor + base;
    }
}

print("Testing lambda with 'this' keyword");

Calculator calc = new Calculator(4, 10);

Processor thisLambda = calc.getThisLambda();
print("This lambda (5): " + thisLambda.process(5));

BinaryProcessor multiThis = calc.getMultipleThisLambda();
print("Multiple this (3, 2): " + multiThis.process(3, 2));

Processor blockThis = calc.getThisBlockLambda();
print("Block this (7): " + blockThis.process(7));

Processor mixed = calc.getMixedAccessLambda();
print("Mixed access (6): " + mixed.process(6));

print("Lambda 'this' keyword test completed");