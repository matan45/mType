import "../../lib/exceptions/Exception.mt";
import "../../lib/exceptions/RuntimeException.mt";

function testSimpleStackTrace(): void {
    try {
        Exception e = new Exception("Test exception");
        throw e;
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        print("Stack trace: " + trace);
    }
}

function testRuntimeExceptionStackTrace(): void {
    try {
        RuntimeException e = new RuntimeException("Runtime error");
        throw e;
    } catch (RuntimeException e) {
        print("Caught runtime exception: " + e.getMessage());
        print("Stack trace: " + e.getStackTrace());
    }
}

// Run tests
print("=== Test 1: Simple stack trace ===");
testSimpleStackTrace();

print("");
print("=== Test 2: RuntimeException stack trace ===");
testRuntimeExceptionStackTrace();

print("");
print("All stack trace tests completed");
