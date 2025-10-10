// Test exception thrown in finally block
import "../../lib/exceptions/Exception.mt";

class CleanupException extends Exception {
    constructor(string msg):super(msg) {
    }
}

class ProcessingException extends Exception {
    constructor(string msg):super(msg) {
    }
}

// Test 1: Exception in finally block overrides return value
function testFinallyOverridesReturn(): string {
    try {
        print("Try: returning success");
        return "success";
    } finally {
        print("Finally: throwing exception");
        throw new CleanupException("Cleanup failed");
    }
}

// Test 2: Exception in finally overrides exception from try
function testFinallyOverridesTryException(): string {
    try {
        try {
            print("Try: throwing ProcessingException");
            ProcessingException e = new ProcessingException("Processing failed");
            throw e;
        } finally {
            print("Finally: throwing CleanupException");
            CleanupException e = new CleanupException("Cleanup failed");
            throw e;
        }
    } catch (CleanupException e) {
        return "Caught CleanupException: " + e.getMessage();
    } catch (ProcessingException e) {
        return "ERROR: Caught ProcessingException (should not reach here)";
    }
    return "No exception caught";
}

// Test 3: Exception in finally overrides exception from catch
function testFinallyOverridesCatchException(): string {
    try {
        try {
            ProcessingException e1 = new ProcessingException("Original error");
            throw e1;
        } catch (ProcessingException e) {
            print("Catch: re-throwing");
            throw e;
        } finally {
            print("Finally: throwing new exception");
            CleanupException e2 = new CleanupException("Finally error");
            throw e2;
        }
    } catch (CleanupException e) {
        return "Caught CleanupException: " + e.getMessage();
    } catch (ProcessingException e) {
        return "ERROR: Caught ProcessingException (should not reach here)";
    }
    return "No exception caught";
}

// Test 4: Normal finally doesn't override successful return
function testFinallyNormalExecution(): string {
    try {
        print("Try: returning success");
        return "success";
    } finally {
        print("Finally: normal cleanup (no exception)");
    }
}

// Test 5: Nested finally blocks with exceptions
function testNestedFinallyExceptions(): string {
    try {
        try {
            try {
                print("Innermost try");
                ProcessingException e = new ProcessingException("Inner error");
                throw e;
            } finally {
                print("Inner finally: throwing exception");
                CleanupException e1 = new CleanupException("Inner cleanup failed");
                throw e1;
            }
        } catch (CleanupException e) {
            print("Middle catch: caught CleanupException");
            throw e;
        } finally {
            print("Middle finally: throwing another exception");
            CleanupException e2 = new CleanupException("Middle cleanup failed");
            throw e2;
        }
    } catch (CleanupException e) {
        return "Outer catch: " + e.getMessage();
    }
    return "No exception caught";
}

// Test 6: Finally with conditional exception
function testConditionalFinallyException(bool shouldThrow): string {
    try {
        print("Try: executing");
        return "success";
    } finally {
        print("Finally: conditional logic");
        if (shouldThrow) {
            print("Finally: throwing exception");
            CleanupException e = new CleanupException("Conditional cleanup failed");
            throw e;
        } else {
            print("Finally: normal completion");
        }
    }
}

// Run all tests
print("=== Test 1: Exception in finally overrides return ===");
try {
    string result = testFinallyOverridesReturn();
    print("ERROR: Got result: " + result);
} catch (CleanupException e) {
    print("Caught: " + e.getMessage());
}

print("\n=== Test 2: Exception in finally overrides try exception ===");
print(testFinallyOverridesTryException());

print("\n=== Test 3: Exception in finally overrides catch exception ===");
print(testFinallyOverridesCatchException());

print("\n=== Test 4: Normal finally doesn't override return ===");
print(testFinallyNormalExecution());

print("\n=== Test 5: Nested finally blocks with exceptions ===");
print(testNestedFinallyExceptions());

print("\n=== Test 6: Conditional finally exception (no throw) ===");
print(testConditionalFinallyException(false));

print("\n=== Test 7: Conditional finally exception (throw) ===");
try {
    string result = testConditionalFinallyException(true);
    print("ERROR: Got result: " + result);
} catch (CleanupException e) {
    print("Caught: " + e.getMessage());
}

print("\nAll exception in finally tests completed");
