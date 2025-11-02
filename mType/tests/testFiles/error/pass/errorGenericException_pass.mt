// Test: Exception class with generic type parameters
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic exception with additional context data
class DataException<T> extends Exception {
    public T data;

    public constructor(string msg, T contextData) {
        super(msg);
        data = contextData;
    }

    public function getData(): T {
        return data;
    }

    public function toString(): string {
        return "DataException: " + message + " [Data: " + data + "]";
    }
}

// Test with integer data
function testIntException(): void {
    try {
        throw new DataException<Int>("Integer error occurred", new Int(404));
    } catch (DataException<Int> e) {
        print("Caught int exception: " + e.getMessage());
        print("Error code: " + e.getData());
    }
}

// Test with string data
function testStringException(): void {
    try {
        throw new DataException<String>("String error occurred", new String("INVALID_INPUT"));
    } catch (DataException<String> e) {
        print("Caught string exception: " + e.getMessage());
        print("Error type: " + e.getData());
    }
}

print("Testing generic exception with Int data:");
testIntException();

print("Testing generic exception with String data:");
testStringException();

print("Generic exception test passed!");
