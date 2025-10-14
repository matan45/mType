// Test file for O1/O2 optimization - Dead Code Elimination in Lambda Functions
// Tests unreachable code after return statements in lambda bodies

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
    Processor<int> doubler = (int x) -> {
        int result = x * 2;
        return result;
        print("Dead code in lambda");  // Should be removed
        int unused = 100;               // Should be removed
    };

    int value = doubler.process(5);
    print("Doubled value: " + value);
}

// Test 2: Lambda with early return in conditional
function testLambdaWithConditionalReturn(): void {
    Predicate<int> isPositive = (int x) -> {
        if (x > 0) {
            return true;
            print("Dead in if branch");  // Should be removed
        } else {
            return false;
            print("Dead in else branch"); // Should be removed
        }
        print("Dead after if-else");      // Should be removed
        return false;                      // Should be removed
    };

    bool result1 = isPositive.test(10);
    bool result2 = isPositive.test(-5);
    print("Positive test: " + result1 + ", " + result2);
}

// Test 3: Nested lambdas with dead code
function testNestedLambdas(): void {
    Function<int, Function<int, int>> multiplier = (int factor) -> {
        print("Creating multiplier with factor: " + factor);
        return (int x) -> {
            int result = x * factor;
            return result;
            print("Dead in inner lambda");  // Should be removed
            factor = factor + 1;             // Should be removed
        };
    };

    Function<int, int> times3 = multiplier.apply(3);
    int result = times3.apply(7);
    print("Multiplication result: " + result);
}

// Test 4: Lambda with multiple returns
function testLambdaWithMultipleReturns(): void {
    Function<int, string> classifier = (int x) -> {
        if (x < 0) {
            return "negative";
        }
        if (x == 0) {
            return "zero";
        }
        if (x > 0) {
            return "positive";
        }
        print("Dead after all returns");  // Should be removed
        return "unknown";                  // Should be removed
    };

    print(classifier.apply(-5));
    print(classifier.apply(0));
    print(classifier.apply(10));
}

// Test 5: Lambda in loop with dead code
function testLambdaInLoop(): void {
    for (int i = 0; i < 3; i = i + 1) {
        Processor<int> processor = (int x) -> {
            int doubled = x * 2;
            return doubled;
            print("Dead in loop lambda");  // Should be removed
        };

        int result = processor.process(i);
        print("Loop result: " + result);
    }
}

// Test 6: Lambda with throw and dead code
import * from "../../lib/exceptions/Exception.mt";

function testLambdaWithThrow(): void {
    Processor<int> validator = (int x) -> {
        if (x < 0) {
            throw new Exception("Negative value not allowed");
            print("Dead after throw");  // Should be removed
            return 0;                    // Should be removed
        }
        return x;
    };

    try {
        int result = validator.process(5);
        print("Valid result: " + result);

        result = validator.process(-1);
        print("This should not print");
    } catch (Exception e) {
        print("Caught exception in lambda");
    }
}

// Test 7: Expression lambda (should not be transformed)
function testExpressionLambda(): void {
    Processor<int> squarer = (int x) -> x * x;  // Expression lambda, no dead code possible
    int result = squarer.process(4);
    print("Squared value: " + result);
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
