// Test: Lambda frames in stack trace
// Expected: Stack traces should include lambda invocation frames
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test 1: Exception thrown inside lambda
function testLambdaException(): void {
    print("=== Test 1: Exception in lambda ===");
    try {
        function<void> throwingLambda = (): void => {
            print("Lambda executing");
            RuntimeException e = new RuntimeException("Exception from lambda");
            throw e;
        };

        print("Calling lambda");
        throwingLambda();
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
    public function triggerEvent(function<void> callback): void {
        print("Triggering event with callback");
        callback();
        print("Event completed");
    }
}

function testLambdaCallback(): void {
    print("=== Test 2: Lambda callback exception ===");
    EventHandler handler = new EventHandler();

    try {
        handler.triggerEvent((): void => {
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
        function<void> outerLambda = (): void => {
            print("Outer lambda executing");

            function<void> innerLambda = (): void => {
                print("Inner lambda executing");
                RuntimeException e = new RuntimeException("Inner lambda error");
                throw e;
            };

            print("Outer calling inner");
            innerLambda();
            print("Outer should not reach here");
        };

        print("Calling outer lambda");
        outerLambda();
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
        function<Int, Int> processor = (Int value): Int => {
            print("Processing value: " + value.toString());
            if (value.toInt() < 0) {
                RuntimeException e = new RuntimeException("Negative value not allowed");
                throw e;
            }
            return new Int(value.toInt() * 2);
        };

        print("Calling lambda with -5");
        Int result = processor(new Int(-5));
        print("Result: " + result.toString());
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
    public function process(Int[] values, function<Int, Int> transform): void {
        print("Processing array with lambda");
        Int i = new Int(0);
        while (i.toInt() < values.length) {
            print("Processing index: " + i.toString());
            Int result = transform(values[i.toInt()]);
            print("Transformed: " + result.toString());
            i = new Int(i.toInt() + 1);
        }
    }
}

function testLambdaInArray(): void {
    print("=== Test 5: Lambda in array processing ===");
    ArrayProcessor processor = new ArrayProcessor();
    Int[] numbers = new Int[3];
    numbers[0] = new Int(10);
    numbers[1] = new Int(0);
    numbers[2] = new Int(30);

    try {
        processor.process(numbers, (Int val): Int => {
            print("Lambda processing: " + val.toString());
            if (val.toInt() == 0) {
                RuntimeException e = new RuntimeException("Cannot process zero");
                throw e;
            }
            return new Int(val.toInt() + 100);
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
    Int capturedValue = new Int(42);

    try {
        function<void> closureLambda = (): void => {
            print("Closure lambda with captured value: " + capturedValue.toString());
            if (capturedValue.toInt() > 40) {
                RuntimeException e = new RuntimeException("Captured value too large");
                throw e;
            }
        };

        print("Calling closure lambda");
        closureLambda();
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
function createThrowingLambda(): function<void> {
    print("Creating throwing lambda");
    return (): void => {
        print("Returned lambda executing");
        RuntimeException e = new RuntimeException("Returned lambda error");
        throw e;
    };
}

function testReturnedLambda(): void {
    print("=== Test 7: Returned lambda exception ===");
    try {
        function<void> lambda = createThrowingLambda();
        print("Executing returned lambda");
        lambda();
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
