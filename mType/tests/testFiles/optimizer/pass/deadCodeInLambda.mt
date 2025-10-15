// Test file for O1/O2 optimization - Dead Code Elimination in Lambda Functions
// Tests unreachable code after return statements in lambda bodies
// Test 6: Lambda with throw and dead code
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Interface for testing
interface Processor<T> {
    function process(T value): T;
}

interface Predicate<T> {
    function test(T value): bool;
}

interface Function<T, R> {
    function apply(T value): R;
}

// Test 1: Lambda with return and dead code
function testLambdaWithReturn(): void {
    Processor<Int> doubler = x -> {
        int result = x.value * 2;
        return new Int(result);
        print("Dead code in lambda");  // Should be removed
        int unused = 100;               // Should be removed
    };

    Int value = doubler.process(new Int(5));
    print("Doubled value: " + value.toString());
}

// Test 2: Lambda with early return in conditional
function testLambdaWithConditionalReturn(): void {
    Predicate<Int> isPositive = x -> {
        if (x.value > 0) {
            return true;
            print("Dead in if branch");  // Should be removed
        } else {
            return false;
            print("Dead in else branch"); // Should be removed
        }
        print("Dead after if-else");      // Should be removed
        return false;                      // Should be removed
    };

    bool result1 = isPositive.test(new Int(10));
    bool result2 = isPositive.test(new Int(-5));
    print("Positive test: " + result1 + ", " + result2);
}

// Test 3: Nested lambdas with dead code
function testNestedLambdas(): void {
    Function<Int, Function<Int, Int>> multiplier = factor -> {
        print("Creating multiplier with factor: " + factor);
        return x -> {
            int result = x.value * factor.value;
            return new Int(result);
            print("Dead in inner lambda");  // Should be removed
            factor = factor + 1;             // Should be removed
        };
    };

    Function<Int, Int> times3 = multiplier.apply(new Int(3));
    Int result = times3.apply(new Int(7));
    print("Multiplication result: " + result.toString());
}

// Test 4: Lambda with multiple returns
function testLambdaWithMultipleReturns(): void {
    Function<Int, String> classifier = x -> {
        if (x.value < 0) {
            return new String("negative");
        }
        if (x.value == 0) {
            return new String("zero");
        }
        if (x.value > 0) {
            return new String("positive");
        }
        print("Dead after all returns");  // Should be removed
        return new String("unknown");                  // Should be removed
    };

    print(classifier.apply(new Int(-5)).toString());
    print(classifier.apply(new Int(0)).toString());
    print(classifier.apply(new Int(10)).toString());
}

// Test 5: Lambda in loop with dead code
function testLambdaInLoop(): void {
    for (int i = 0; i < 3; i = i + 1) {
        Processor<Int> processor = x -> {
            int doubled = x.value * 2;
            return new Int(doubled);
            print("Dead in loop lambda");  // Should be removed
        };

        Int result = processor.process(new Int(i));
        print("Loop result: " + result.toString());
    }
}

function testLambdaWithThrow(): void {
    Processor<Int> validator = x -> {
        if (x.value < 0) {
            throw new Exception("Negative value not allowed");
            print("Dead after throw");  // Should be removed
            return 0;                    // Should be removed
        }
        return x;
    };

    try {
        Int result = validator.process(new Int(5));
        print("Valid result: " + result);

        result = validator.process(new Int(-1));
        print("This should not print");
    } catch (Exception e) {
        print("Caught exception in lambda");
    }
}

// Test 7: Expression lambda (should not be transformed)
function testExpressionLambda(): void {
    Processor<Int> squarer = x -> new Int(x.value * x.value);  // Expression lambda, no dead code possible
    Int result = squarer.process(new Int(4));
    print("Squared value: " + result.toString());
}

// Entry point
print("=== Testing Lambda Dead Code Elimination ===");

testLambdaWithReturn();
testLambdaWithConditionalReturn();
testNestedLambdas();
testLambdaWithMultipleReturns();
testLambdaInLoop();
testLambdaWithThrow();
testExpressionLambda();

print("Lambda Dead Code Test Complete!");
