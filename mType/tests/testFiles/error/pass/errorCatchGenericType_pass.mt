// Test: Catch block with generic exception
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic result exception
class ResultException<T> extends Exception {
    public T result;
    public bool hasResult;

    public constructor(string msg) {
        super(msg);
        hasResult = false;
    }

    public constructor(string msg, T res) {
        super(msg);
        result = res;
        hasResult = true;
    }

    public function getResult(): T {
        return result;
    }

    public function hasValue(): bool {
        return hasResult;
    }
}

// Test catching specific generic exception types
function testCatchGenericInt(): void {
    try {
        throw new ResultException<Int>("Operation failed with result", new Int(500));
    } catch (ResultException<Int> e) {
        print("Caught ResultException<Int>: " + e.getMessage());
        if (e.hasValue()) {
            print("Result value: " + e.getResult());
        }
    }
}

function testCatchGenericString(): void {
    try {
        throw new ResultException<String>("Operation failed with message", new String("Timeout"));
    } catch (ResultException<String> e) {
        print("Caught ResultException<String>: " + e.getMessage());
        if (e.hasValue()) {
            print("Result message: " + e.getResult());
        }
    }
}

// Test catching base exception
function testCatchAsBase(): void {
    try {
        throw new ResultException<Int>("Generic error", new Int(404));
    } catch (Exception e) {
        print("Caught as base Exception: " + e.getMessage());
    }
}

print("Testing catch with generic Int exception:");
testCatchGenericInt();

print("Testing catch with generic String exception:");
testCatchGenericString();

print("Testing catch as base exception:");
testCatchAsBase();

print("Catch generic type test passed!");
