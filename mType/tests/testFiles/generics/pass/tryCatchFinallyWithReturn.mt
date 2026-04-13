// Test try-catch-finally with return statements in generic methods
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
class Result<T> {
    T value;
    bool hasError;
    String errorMessage;

    constructor(T val, bool error, String msg) {
        this.value = val;
        this.hasError = error;
        this.errorMessage = msg;
    }

    public function getValue(): T {
        return this.value;
    }

    public function getHasError(): bool {
        return this.hasError;
    }
}

class GenericProcessor<T> {
    T data;

    constructor(T initialData) {
        this.data = initialData;
    }

    // Test 1: Generic method with try-catch-finally and return
    public function processWithFinally(T? input): Result<T> {
        try {
            print("Processing input");
            if (input == null) {
                Exception e = new Exception("Null input");
                throw e;
            }
            if (input != null) {
                return new Result<T>(input, false, new String(""));
            }
            return new Result<T>(this.data, false, new String(""));
        } catch (Exception e) {
            print("Error: " + e.getMessage());
            return new Result<T>(this.data, true, new String(e.getMessage()));
        } finally {
            print("Finally: cleanup");
        }
    }

    // Test 2: Nested try-catch-finally with generic return
    public function nestedProcessing(T input, bool throwError): Result<T> {
        try {
            print("Outer try");
            try {
                print("Inner try");
                if (throwError) {
                    Exception e = new Exception("Inner error");
                    throw e;
                }
                return new Result<T>(input, false, new String(""));
            } catch (Exception e) {
                print("Inner catch: " + e.getMessage());
                return new Result<T>(this.data, true, new String("Inner: " + e.getMessage()));
            } finally {
                print("Inner finally");
            }
        } catch (Exception e) {
            print("Outer catch: " + e.getMessage());
            return new Result<T>(this.data, true, new String("Outer: " + e.getMessage()));
        } finally {
            print("Outer finally");
        }
    }
}

// Static generic function with try-catch-finally
function <U> safeCast(U value, bool shouldFail): Result<U> {
    try {
        if (shouldFail) {
            Exception e = new Exception("Cast failed");
            throw e;
        }
        return new Result<U>(value, false, new String(""));
    } catch (Exception e) {
        print("safeCast error: " + e.getMessage());
        return new Result<U>(value, true, new String(e.getMessage()));
    } finally {
        print("safeCast finally");
    }
}

// Test 1: Generic processor with Int
print("=== Test 1: Generic processor with Int ===");
GenericProcessor<Int> processor1 = new GenericProcessor<Int>(new Int(100));
Result<Int> result1 = processor1.processWithFinally(new Int(50));
print("Result 1 hasError: " + result1.getHasError());
print("Result 1 value: " + result1.getValue().toString());

Result<Int> result2 = processor1.processWithFinally(null);
print("Result 2 hasError: " + result2.getHasError());
print("Result 2 value: " + result2.getValue().toString());

// Test 2: Nested processing with String
print("\n=== Test 2: Nested processing with String ===");
GenericProcessor<String> processor2 = new GenericProcessor<String>(new String("default"));
Result<String> result3 = processor2.nestedProcessing(new String("test"), false);
print("Result 3 hasError: " + result3.getHasError());
print("Result 3 value: " + result3.getValue().toString());

Result<String> result4 = processor2.nestedProcessing(new String("error"), true);
print("Result 4 hasError: " + result4.getHasError());
print("Result 4 value: " + result4.getValue().toString());

// Test 3: Static generic function
print("\n=== Test 3: Static generic function ===");
Result<Int> result5 = safeCast<Int>(new Int(42), false);
print("Result 5 hasError: " + result5.getHasError());
print("Result 5 value: " + result5.getValue().toString());

Result<String> result6 = safeCast<String>(new String("test"), true);
print("Result 6 hasError: " + result6.getHasError());
print("Result 6 value: " + result6.getValue().toString());

print("\nAll generic try-catch-finally tests completed");
