// Integration Test 10: Exception Hierarchy with Finally
// Tests: Multiple catch blocks ordered + Finally overriding returns
import * from "../../lib/exceptions/Exception.mt";
// Exception hierarchy
class BaseException extends Exception {
    constructor(string msg): super(msg) {
    }
}

class RuntimeException extends BaseException {
    constructor(string msg) : super(msg) {}
}

class IOException extends BaseException {
    private int errorCode;

    constructor(string msg, int code) : super(msg) {
        this.errorCode = code;
    }

    public function getErrorCode(): int {
        return this.errorCode;
    }
}

class NetworkException extends IOException {
    constructor(string msg) : super(msg, 500) {}
}

class TimeoutException extends NetworkException {
    constructor(string msg) : super(msg) {}
}

// Test finally overriding return values
function finallyOverridesReturn(): int {
    try {
        print("In try block");
        return 10;  // This will be ignored
    } finally {
        print("In finally block");
        return 20;  // This value is actually returned
    }
}

// Test finally with exception suppression
function finallySuppressesException(): int {
    try {
        print("Throwing exception");
        RuntimeException ex = new RuntimeException("This will be suppressed");
        throw ex;
    } finally {
        print("Finally suppresses exception");
        return 30;  // Suppresses the exception and returns
    }
}

// Test multiple catch blocks in order
function multipleCatchBlocks(int testCase): string {
    try {
        print("Test case: " + testCase);

        if (testCase == 1) {
            TimeoutException ex = new TimeoutException("Timeout occurred");
            throw ex;
        } else if (testCase == 2) {
            NetworkException ex = new NetworkException("Network failed");
            throw ex;
        } else if (testCase == 3) {
            IOException ex = new IOException("IO failed", 404);
            throw ex;
        } else if (testCase == 4) {
            RuntimeException ex = new RuntimeException("Runtime error");
            throw ex;
        } else {
            BaseException ex = new BaseException("Base error");
            throw ex;
        }

    } catch (TimeoutException e) {
        print("Caught TimeoutException: " + e.getMessage());
        return "timeout";
    } catch (NetworkException e) {
        print("Caught NetworkException: " + e.getMessage());
        return "network";
    } catch (IOException e) {
        print("Caught IOException: " + e.getMessage() + " (code: " + e.getErrorCode() + ")");
        return "io";
    } catch (RuntimeException e) {
        print("Caught RuntimeException: " + e.getMessage());
        return "runtime";
    } catch (BaseException e) {
        print("Caught BaseException: " + e.getMessage());
        return "base";
    } finally {
        print("Finally block executed");
    }
}

// Test finally without return (normal behavior)
function finallyNoReturn(): int {
    int result = 0;

    try {
        print("Try block");
        result = 50;
        return result;  // This return value is used
    } finally {
        print("Finally block");
        result = 60;  // Modifies variable but doesn't override return
    }
}

// Test nested try-catch-finally
function nestedTryCatchFinally(): int {
    int result = 0;

    try {
        print("Outer try");

        try {
            print("Inner try");
            RuntimeException ex = new RuntimeException("Inner exception");
            throw ex;
        } catch (RuntimeException e) {
            print("Inner catch: " + e.getMessage());
            result = 100;
        } finally {
            print("Inner finally");
        }

        print("After inner try-catch");
        return result;

    } catch (BaseException e) {
        print("Outer catch (unreachable)");
        return -1;
    } finally {
        print("Outer finally");
    }
}

// Test exception rethrowing with finally
function exceptionRethrowWithFinally(): int {
    try {
        print("Try: throwing exception");
        RuntimeException ex = new RuntimeException("Original exception");
        throw ex;
    } catch (RuntimeException e) {
        print("Catch: " + e.getMessage());
        print("Rethrowing");
        throw e;  // Rethrow
    } finally {
        print("Finally: cleanup before rethrow");
    }
}

// Test finally in loop
function finallyInLoop(): void {
    print("--- Finally in loop ---");

    for (int i = 0; i < 3; i = i + 1) {
        try {
            print("Loop iteration: " + i);

            if (i == 1) {
                RuntimeException ex = new RuntimeException("Exception at i=1");
                throw ex;
            }

            print("No exception for i=" + i);

        } catch (RuntimeException e) {
            print("Caught: " + e.getMessage());
        } finally {
            print("Finally for i=" + i);
        }
    }
}

// Main test execution
print("=== Test 10: Exception Hierarchy with Finally ===");

// Test 1: Finally overrides return
print("--- Finally overrides return ---");
int r1 = finallyOverridesReturn();
print("Result: " + r1);

// Test 2: Finally suppresses exception
print("--- Finally suppresses exception ---");
int r2 = finallySuppressesException();
print("Result: " + r2);

// Test 3: Multiple catch blocks
print("--- Multiple catch blocks (ordered) ---");
string c1 = multipleCatchBlocks(1);
print("Result: " + c1);

print("---");
string c2 = multipleCatchBlocks(2);
print("Result: " + c2);

print("---");
string c3 = multipleCatchBlocks(3);
print("Result: " + c3);

print("---");
string c4 = multipleCatchBlocks(4);
print("Result: " + c4);

// Test 4: Finally without return
print("--- Finally without return ---");
int r3 = finallyNoReturn();
print("Result: " + r3);

// Test 5: Nested try-catch-finally
print("--- Nested try-catch-finally ---");
int r4 = nestedTryCatchFinally();
print("Result: " + r4);

// Test 6: Exception rethrow with finally
print("--- Exception rethrow with finally ---");
try {
    int r5 = exceptionRethrowWithFinally();
    print("Result: " + r5);
} catch (RuntimeException e) {
    print("Outer catch: " + e.getMessage());
}

// Test 7: Finally in loop
finallyInLoop();

print("=== Test 10 Complete ===");
