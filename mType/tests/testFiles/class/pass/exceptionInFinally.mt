// Test exception thrown in finally block using classes
import * from "../../lib/exceptions/Exception.mt";

class CleanupException extends Exception {
    constructor(string msg):super(msg) {
    }
}

class ProcessingException extends Exception {
    constructor(string msg):super(msg) {
    }
}

class ExceptionFinallyTests {
    // Test 1: Exception in finally block overrides return value
    public function testFinallyOverridesReturn(): string {
        try {
            print("Try: returning success");
            return "success";
        } finally {
            print("Finally: throwing exception");
            throw new CleanupException("Cleanup failed");
        }
    }

    // Test 2: Exception in finally overrides exception from try
    public function testFinallyOverridesTryException(): string {
        try {
            try {
                print("Try: throwing ProcessingException");
                throw new ProcessingException("Processing failed");
            } finally {
                print("Finally: throwing CleanupException");
                throw new CleanupException("Cleanup failed");
            }
        } catch (CleanupException e) {
            return "Caught CleanupException: " + e.getMessage();
        } catch (ProcessingException e) {
            return "ERROR: Caught ProcessingException (should not reach here)";
        }
        return "No exception caught";
    }

    // Test 3: Exception in finally overrides exception from catch
    public function testFinallyOverridesCatchException(): string {
        try {
            try {
                throw new ProcessingException("Original error");
            } catch (ProcessingException e) {
                print("Catch: re-throwing");
                throw e;
            } finally {
                print("Finally: throwing new exception");
                throw new CleanupException("Finally error");
            }
        } catch (CleanupException e) {
            return "Caught CleanupException: " + e.getMessage();
        } catch (ProcessingException e) {
            return "ERROR: Caught ProcessingException (should not reach here)";
        }
        return "No exception caught";
    }

    // Test 4: Normal finally doesn't override successful return
    public function testFinallyNormalExecution(): string {
        try {
            print("Try: returning success");
            return "success";
        } finally {
            print("Finally: normal cleanup (no exception)");
        }
    }
}

// Run all tests
print("=== Test 1: Exception in finally overrides return ===");
ExceptionFinallyTests tests = new ExceptionFinallyTests();
try {
    string result = tests.testFinallyOverridesReturn();
    print("ERROR: Got result: " + result);
} catch (CleanupException e) {
    print("Caught: " + e.getMessage());
}

print("\n=== Test 2: Exception in finally overrides try exception ===");
print(tests.testFinallyOverridesTryException());

print("\n=== Test 3: Exception in finally overrides catch exception ===");
print(tests.testFinallyOverridesCatchException());

print("\n=== Test 4: Normal finally doesn't override return ===");
print(tests.testFinallyNormalExecution());

print("\nAll exception in finally tests completed");
