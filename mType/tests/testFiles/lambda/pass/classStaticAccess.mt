// Test lambda accessing static fields and qualified static access
interface Function {
    function apply(int x) : int;
}

class Math {
    static int PI_TIMES_100 = 314;
    static int E_TIMES_100 = 271;
}

class Calculator {
    static int MULTIPLIER = 5;
    int instanceValue;

    constructor(int value) {
        instanceValue = value;
    }

    function getStaticLambda() : Function {
        // Lambda accessing static field (unqualified)
        return x -> x * MULTIPLIER;
    }

    function getQualifiedStaticLambda() : Function {
        // Lambda accessing qualified static field
        return x -> x + Math::PI_TIMES_100;
    }

    function getComplexStaticLambda() : Function {
        // Lambda mixing static, qualified static, and instance
        return x -> x * MULTIPLIER + Math::E_TIMES_100 + instanceValue;
    }

    static function getStaticMethodLambda() : Function {
        // Lambda from static method accessing static field
        return x -> x * MULTIPLIER + Math::PI_TIMES_100;
    }
}

print("Testing lambda static field access");

Calculator calc = new Calculator(20);

Function staticLambda = calc.getStaticLambda();
print("Static field (3): " + staticLambda.apply(3));

Function qualifiedLambda = calc.getQualifiedStaticLambda();
print("Qualified static (2): " + qualifiedLambda.apply(2));

Function complexLambda = calc.getComplexStaticLambda();
print("Complex static (4): " + complexLambda.apply(4));

Function staticMethodLambda = Calculator::getStaticMethodLambda();
print("Static method lambda (1): " + staticMethodLambda.apply(1));

print("Lambda static access test completed");