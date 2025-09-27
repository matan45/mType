// Test lambda capturing method parameters
interface Function {
    function apply(int x) : int;
}

interface BinaryFunction {
    function apply(int a, int b) : int;
}

class ParameterCapture {
    int classField;

    constructor(int field) {
        classField = field;
    }

    function createLambdaWithParam(int factor) : Function {
        // Lambda capturing method parameter
        return x -> x * factor;
    }

    function createMixedCaptureLambda(int multiplier, int offset) : Function {
        // Lambda capturing multiple parameters and class field
        return x -> (x + offset) * multiplier + classField;
    }

    function createComplexLambda(int a, int b) : BinaryFunction {
        // Lambda capturing parameters in binary function
        return (x, y) -> x * a + y * b + classField;
    }

    function createBlockLambdaWithParams(int base, int increment) : Function {
        // Block lambda with parameter capture
        return x -> {
            int temp = x + base;
            temp = temp * increment;
            return temp + classField;
        };
    }
}

print("Testing lambda method parameter capture");

ParameterCapture obj = new ParameterCapture(100);

Function paramLambda = obj.createLambdaWithParam(7);
print("Parameter capture (3): " + paramLambda.apply(3));

Function mixedLambda = obj.createMixedCaptureLambda(2, 5);
print("Mixed capture (4): " + mixedLambda.apply(4));

BinaryFunction complexLambda = obj.createComplexLambda(3, 4);
print("Complex capture (2, 5): " + complexLambda.apply(2, 5));

Function blockLambda = obj.createBlockLambdaWithParams(10, 2);
print("Block parameter capture (6): " + blockLambda.apply(6));

print("Lambda parameter capture test completed");