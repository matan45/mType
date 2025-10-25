// Test: Lambda accessing super members
// Expected: Should compile and run successfully

interface Function {
    function apply(int x): string;
}

interface IntFunction {
    function apply(int x): int;
}

class Parent {
    protected int multiplier;
    protected string prefix;

    constructor(int mult, string pre) {
        this.multiplier = mult;
        this.prefix = pre;
    }

    public function format(int value): string {
        return this.prefix + ": " + (value * this.multiplier);
    }

    public function calculate(int value): int {
        return value * this.multiplier;
    }
}

class Child extends Parent {
    private string childPrefix;

    constructor(int mult, string pre, string childPre): super(mult, pre) {
        this.childPrefix = childPre;
    }

    public function format(int value): string {
        return this.childPrefix + " -> " + super.format(value);
    }

    // Lambda accessing super method
    public function getLambdaWithSuperMethod(): Function {
        return x -> super.format(x) + " (from lambda)";
    }

    // Lambda accessing super field
    public function getLambdaWithSuperField(): IntFunction {
        return x -> x * super.multiplier;
    }

    // Block lambda accessing super
    public function getBlockLambdaWithSuper(): Function {
        return x -> {
            string parentResult = super.format(x);
            int calculated = super.calculate(x);
            return "Lambda: " + parentResult + ", calc=" + calculated;
        };
    }

    // Nested lambda with super access
    public function getNestedLambdaWithSuper(): Function {
        int localVar = 10;
        return x -> {
            IntFunction inner = y -> y + super.multiplier + localVar;
            int innerResult = inner.apply(x);
            return super.format(innerResult);
        };
    }

    // Lambda accessing both super and this
    public function getMixedLambda(): Function {
        return x -> {
            string superPart = super.format(x);
            string childPart = this.childPrefix + ": " + x;
            return superPart + " | " + childPart;
        };
    }
}

function main(): void {
    print("Testing lambda with super access");

    Child child = new Child(5, "Parent", "Child");

    // Test simple lambda with super method
    Function lambdaMethod = child.getLambdaWithSuperMethod();
    print(lambdaMethod.apply(3));

    // Test lambda with super field
    IntFunction lambdaField = child.getLambdaWithSuperField();
    print("Lambda field result: " + lambdaField.apply(7));

    // Test block lambda with super
    Function blockLambda = child.getBlockLambdaWithSuper();
    print(blockLambda.apply(4));

    // Test nested lambda with super
    Function nestedLambda = child.getNestedLambdaWithSuper();
    print(nestedLambda.apply(2));

    // Test mixed lambda
    Function mixedLambda = child.getMixedLambda();
    print(mixedLambda.apply(6));

    print("Lambda with super access test completed!");
}

main();
