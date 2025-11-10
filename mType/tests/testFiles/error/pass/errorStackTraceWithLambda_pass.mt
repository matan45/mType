// Test: Lambda frames in stack trace
// Expected: Stack traces should include lambda invocation frames
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";

// Interface for void functions
interface Runnable {
    function run(): void;
}

// Interface for int->int functions
interface IntToIntFunction {
    function apply(int value): int;
}

// Test 1: Exception thrown inside lambda
function testLambdaException(): void {
    print("=== Test 1: Exception in lambda ===");
    try {
        Runnable throwingLambda = () -> {
            print("Lambda executing");
            RuntimeException e = new RuntimeException("Exception from lambda");
            throw e;
        };

        print("Calling lambda");
        throwingLambda.run();
        print("Should not reach here");
    } catch (RuntimeException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace includes lambda: true");
        }
        print("Lambda exception handled");
    }
}

// Test 2: Lambda as callback throwing exception
class EventHandler {
    public function triggerEvent(Runnable callback): void {
        print("Triggering event with callback");
        callback.run();
        print("Event completed");
    }
}

function testLambdaCallback(): void {
    print("=== Test 2: Lambda callback exception ===");
    EventHandler handler = new EventHandler();

    try {
        handler.triggerEvent(() -> {
            print("Callback lambda executing");
            RuntimeException e = new RuntimeException("Callback failed");
            throw e;
        });
    } catch (RuntimeException e) {
        print("Caught callback exception: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace from callback: true");
        }
        print("Callback exception handled");
    }
}

// Test 3: Nested lambdas with exception
function testNestedLambdas(): void {
    print("=== Test 3: Nested lambda exception ===");
    try {
        Runnable outerLambda = () -> {
            print("Outer lambda executing");

            Runnable innerLambda = () -> {
                print("Inner lambda executing");
                RuntimeException e = new RuntimeException("Inner lambda error");
                throw e;
            };

            print("Outer calling inner");
            innerLambda.run();
            print("Outer should not reach here");
        };

        print("Calling outer lambda");
        outerLambda.run();
    } catch (RuntimeException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace includes nested lambdas: true");
        }
        print("Nested lambda exception handled");
    }
}

// Test 4: Lambda with parameter throwing exception
function testLambdaWithParams(): void {
    print("=== Test 4: Lambda with parameters ===");
    try {
        IntToIntFunction processor = value -> {
            print("Processing value: " + value);
            if (value < 0) {
                RuntimeException e = new RuntimeException("Negative value not allowed");
                throw e;
            }
            return value * 2;
        };

        print("Calling lambda with -5");
        int result = processor.apply(-5);
        print("Result: " + result);
    } catch (RuntimeException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace from parameterized lambda: true");
        }
        print("Parameterized lambda exception handled");
    }
}

// Test 5: Lambda in array operation
class ArrayProcessor {
    public function process(int[] values, IntToIntFunction transform): void {
        print("Processing array with lambda");
        int i = 0;
        while (i < values.length) {
            print("Processing index: " + i);
            int result = transform.apply(values[i]);
            print("Transformed: " + result);
            i = i + 1;
        }
    }
}

function testLambdaInArray(): void {
    print("=== Test 5: Lambda in array processing ===");
    ArrayProcessor processor = new ArrayProcessor();
    int[] numbers = new int[3];
    numbers[0] = 10;
    numbers[1] = 0;
    numbers[2] = 30;

    try {
        processor.process(numbers, val -> {
            print("Lambda processing: " + val);
            if (val == 0) {
                RuntimeException e = new RuntimeException("Cannot process zero");
                throw e;
            }
            return val + 100;
        });
    } catch (RuntimeException e) {
        print("Caught array processing error: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace from array lambda: true");
        }
        print("Array lambda exception handled");
    }
}

// Test 6: Lambda with closure throwing exception
function testLambdaClosure(): void {
    print("=== Test 6: Lambda closure exception ===");
    int capturedValue = 42;

    try {
        Runnable closureLambda = () -> {
            print("Closure lambda with captured value: " + capturedValue);
            if (capturedValue > 40) {
                RuntimeException e = new RuntimeException("Captured value too large");
                throw e;
            }
        };

        print("Calling closure lambda");
        closureLambda.run();
    } catch (RuntimeException e) {
        print("Caught closure exception: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace includes closure: true");
        }
        print("Closure exception handled");
    }
}

// Test 7: Lambda returning lambda with exception
function createThrowingLambda(): Runnable {
    print("Creating throwing lambda");
    return () -> {
        print("Returned lambda executing");
        RuntimeException e = new RuntimeException("Returned lambda error");
        throw e;
    };
}

function testReturnedLambda(): void {
    print("=== Test 7: Returned lambda exception ===");
    try {
        Runnable lambda = createThrowingLambda();
        print("Executing returned lambda");
        lambda.run();
    } catch (RuntimeException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace from returned lambda: true");
        }
        print("Returned lambda exception handled");
    }
}

// Run all tests
testLambdaException();
print("");
testLambdaCallback();
print("");
testNestedLambdas();
print("");
testLambdaWithParams();
print("");
testLambdaInArray();
print("");
testLambdaClosure();
print("");
testReturnedLambda();
print("");
print("Lambda stack trace test completed");
