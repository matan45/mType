// Test async lambda functions
import * from "../../lib/primitives/Int.mt";
class Data {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

// Interface for async operations
interface AsyncFunction {
    function async apply(int x): Promise<Int>;
}

interface AsyncDataFunction {
    function async apply(int x): Promise<Data>;
}

interface AsyncBinaryFunction {
    function async compute(int x, int y): Promise<Int>;
}

print("=== Async Lambda Functions Test ===");

// Test 1: Simple async lambda with interface
function async testSimpleAsyncLambda(): Promise<Int> {
    // Define an async lambda
    AsyncFunction asyncLambda = async x -> {
        int result = x * 2;
        return new Int(result);
    };

    // Call and await the async lambda
    Int value = await asyncLambda.apply(5);
    print("Simple async lambda result: " + value);
    return value;
}

// Test 2: Async lambda with object return
function async testAsyncLambdaWithObject(): Promise<Data> {
    // Define an async lambda that returns an object
    AsyncDataFunction asyncObjectLambda = async x -> {
        Data d = new Data(x * 3);
        return d;
    };

    // Call and await the async lambda
    Data result = await asyncObjectLambda.apply(10);
    print("Async lambda object result: " + result.getValue());
    return result;
}

// Test 3: Async lambda as parameter
function async processWithAsyncCallback(
    int value,
    AsyncFunction callback
): Promise<Int> {
    print("Processing value: " + value);
    Int result = await callback.apply(value);
    return result;
}

function async testAsyncLambdaAsParameter(): Promise<Int> {
    // Pass async lambda as argument
    Int result = await processWithAsyncCallback(7, async x -> {
        int doubled = x * 2;
        return new Int(doubled);
    });
    print("Async lambda as parameter result: " + result.toString());
    return result;
}


// Test 4: Async lambda with multiple parameters
function async testAsyncLambdaMultipleParams(): Promise<Int> {
    // Define an async lambda with two parameters
    AsyncBinaryFunction addLambda = async (x, y) -> {
        int sum = x + y;
        return new Int(sum);
    };

    // Call and await the async lambda with two arguments
    Int result = await addLambda.compute(15, 25);
    print("Async lambda with multiple params result: " + result);
    return result;
}

// Test 5: Async lambda expression (single line)
function async testAsyncLambdaExpression(): Promise<Int> {
    AsyncFunction squareLambda = async x -> new Int(x * x);

    Int result = await squareLambda.apply(6);
    print("Async lambda expression result: " + result);
    return result;
}

// Test 6: Inline async lambda with multiple parameters
function async processWithBinaryCallback(
    int a,
    int b,
    AsyncBinaryFunction callback
): Promise<Int> {
    print("Processing values: " + a + " and " + b);
    Int result = await callback.compute(a, b);
    return result;
}

function async testInlineMultiParamLambda(): Promise<Int> {
    // Pass async lambda with multiple parameters as inline argument
    Int result = await processWithBinaryCallback(10, 20, async (x, y) -> {
        int product = x * y;
        return new Int(product);
    });
    print("Inline multi-param async lambda result: " + result);
    return result;
}

// Main function to run all tests
function async main(): Promise<void> {
    Int r1 = await testSimpleAsyncLambda();
    Data r2 = await testAsyncLambdaWithObject();
    Int r3 = await testAsyncLambdaAsParameter();
    Int r4 = await testAsyncLambdaMultipleParams();
    Int r5 = await testAsyncLambdaExpression();
    Int r6 = await testInlineMultiParamLambda();

    print("All async lambda tests completed");
    return null;
}

main();
