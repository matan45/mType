// Comprehensive lambda and class integration test
interface Function {
    function apply(int x) : int;
}

interface BinaryFunction {
    function apply(int a, int b) : int;
}

class Math {
    public static int CONSTANT = 100;
}

class ComplexCalculator {
    static int STATIC_MULTIPLIER = 2;
    int instanceBase;
    int instanceMultiplier;

    constructor(int base, int mult) {
        instanceBase = base;
        instanceMultiplier = mult;
    }

    public function createComprehensiveLambda(int methodParam) : Function {
        // Lambda using all types of access: static, qualified static, instance, this, parameter
        return x -> {
            int temp = x + this.instanceBase;           // this access
            temp = temp * instanceMultiplier;           // direct field access
            temp = temp + methodParam;                  // parameter capture
            temp = temp * STATIC_MULTIPLIER;            // static field access
            return temp + Math::CONSTANT;               // qualified static access
        };
    }

    public function createNestedLambdaContext(int outer) : Function {
        // Method that creates lambda which creates another operation
        int localVar = outer * 2;

        return x -> {
            int intermediate = x + localVar + this.instanceBase;
            intermediate = intermediate * this.instanceMultiplier;
            return intermediate + Math::CONSTANT + STATIC_MULTIPLIER;
        };
    }

    public static function createStaticContextLambda() : BinaryFunction {
        // Lambda from static method
        return (a, b) -> a * STATIC_MULTIPLIER + b + Math::CONSTANT;
    }

    public function getDynamicLambdaGenerator(int seed) : Function {
        // Returns lambda that generates other lambdas (functional programming style)
        return x -> {
            int result = x * seed;
            result = result + this.instanceBase;
            result = result * STATIC_MULTIPLIER;
            return result + Math::CONSTANT;
        };
    }
}

print("Testing comprehensive lambda-class integration");

ComplexCalculator calc = new ComplexCalculator(5, 3);

Function comprehensive = calc.createComprehensiveLambda(10);
print("Comprehensive lambda (4): " + comprehensive.apply(4));

Function nested = calc.createNestedLambdaContext(7);
print("Nested context (6): " + nested.apply(6));

BinaryFunction staticLambda = ComplexCalculator::createStaticContextLambda();
print("Static context (3, 8): " + staticLambda.apply(3, 8));

Function dynamic = calc.getDynamicLambdaGenerator(4);
print("Dynamic generator (5): " + dynamic.apply(5));

print("Complex integration test completed");