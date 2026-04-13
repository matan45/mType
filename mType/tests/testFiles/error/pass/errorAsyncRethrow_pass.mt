// Test: Re-throwing exceptions in async context
// Expected: Exceptions can be caught, processed, and re-thrown correctly
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

class LoggedException extends Exception {
    public bool wasLogged;

    public constructor(string msg): super(msg) {
        wasLogged = false;
    }

    public function markLogged(): void {
        wasLogged = true;
    }

    public function isLogged(): bool {
        return wasLogged;
    }
}

// Test 1: Basic rethrow
function async basicRethrow(): Promise<String> {
    try {
        print("About to throw");
        Exception e = new Exception("Original exception");
        throw e;
        return new String("success");
    } catch (Exception e) {
        print("Caught and rethrowing: " + e.getMessage());
        throw e;
    }
}

function async catchRethrown(): Promise<String> {
    try {
        Promise<String> p = basicRethrow();
        String result = await p;
        return result;
    } catch (Exception e) {
        print("Caught rethrown: " + e.getMessage());
        return new String("handled");
    }
}

// Test 2: Rethrow with modification
function async rethrowWithLogging(): Promise<Int> {
    try {
        print("Creating logged exception");
        LoggedException e = new LoggedException("Error to log");
        throw e;
        return new Int(1);
    } catch (LoggedException e) {
        print("Logging exception: " + e.getMessage());
        e.markLogged();
        print("Logged status: " + e.isLogged());
        throw e;
    }
}

function async handleLogged(): Promise<String> {
    try {
        Promise<Int> p = rethrowWithLogging();
        Int value = await p;
        return new String("unexpected");
    } catch (LoggedException e) {
        print("Received logged exception: " + e.getMessage());
        print("Was logged: " + e.isLogged());
        return new String("logged and handled");
    }
}

// Test 3: Transform and rethrow
function async transformException(): Promise<String> {
    try {
        print("Throwing runtime exception");
        RuntimeException e = new RuntimeException("Runtime error");
        throw e;
        return new String("ok");
    } catch (RuntimeException e) {
        print("Transforming: " + e.getMessage());
        Exception wrapped = new Exception("Wrapped: " + e.getMessage());
        throw wrapped;
    }
}

function async catchTransformed(): Promise<String> {
    try {
        Promise<String> p = transformException();
        String result = await p;
        return result;
    } catch (Exception e) {
        print("Caught transformed: " + e.getMessage());
        return new String("transformation handled");
    }
}

// Test 4: Conditional rethrow
function async conditionalRethrow(bool shouldRethrow): Promise<String> {
    try {
        print("Testing conditional rethrow");
        Exception e = new Exception("Conditional error");
        throw e;
        return new String("success");
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
        if (shouldRethrow) {
            print("Rethrowing");
            throw e;
        }
        print("Not rethrowing");
        return new String("handled locally");
    }
}

function async testConditional(bool shouldRethrow): Promise<String> {
    try {
        Promise<String> p = conditionalRethrow(shouldRethrow);
        String result = await p;
        return new String("result: " + result.toString());
    } catch (Exception e) {
        return new String("caught rethrow: " + e.getMessage());
    }
}

// Test 5: Rethrow in nested try-catch
function async nestedRethrow(): Promise<String> {
    try {
        print("Outer try");
        try {
            print("Inner try");
            Exception e = new Exception("Inner exception");
            throw e;
        } catch (Exception e) {
            print("Inner catch: " + e.getMessage());
            print("Rethrowing from inner");
            throw e;
        }
        return new String("unreachable");
    } catch (Exception e) {
        print("Outer catch: " + e.getMessage());
        return new String("outer handled");
    }
}

// Test 6: Multiple rethrows through chain
function async level1Throw(): Promise<Int> {
    print("Level 1: Throwing");
    Exception e = new Exception("Level 1 error");
    throw e;
    return new Int(1);
}

function async level2Rethrow(): Promise<Int> {
    try {
        print("Level 2: Calling level 1");
        Promise<Int> p = level1Throw();
        Int result = await p;
        return result;
    } catch (Exception e) {
        print("Level 2: Caught and rethrowing");
        throw e;
    }
}

function async level3Rethrow(): Promise<Int> {
    try {
        print("Level 3: Calling level 2");
        Promise<Int> p = level2Rethrow();
        Int result = await p;
        return result;
    } catch (Exception e) {
        print("Level 3: Caught and rethrowing");
        throw e;
    }
}

function async level4Catch(): Promise<String> {
    try {
        print("Level 4: Calling level 3");
        Promise<Int> p = level3Rethrow();
        Int result = await p;
        return new String("unexpected");
    } catch (Exception e) {
        print("Level 4: Final catch - " + e.getMessage());
        return new String("chain complete");
    }
}

function async main(): Promise<void> {
    print("=== Test 1: Basic rethrow ===");
    Promise<String> p1 = catchRethrown();
    String result1 = await p1;
    print("Result: " + result1.toString());

    print("");
    print("=== Test 2: Rethrow with logging ===");
    Promise<String> p2 = handleLogged();
    String result2 = await p2;
    print("Result: " + result2.toString());

    print("");
    print("=== Test 3: Transform and rethrow ===");
    Promise<String> p3 = catchTransformed();
    String result3 = await p3;
    print("Result: " + result3.toString());

    print("");
    print("=== Test 4: Conditional rethrow (true) ===");
    Promise<String> p4 = testConditional(true);
    String result4 = await p4;
    print("Result: " + result4.toString());

    print("");
    print("=== Test 5: Conditional rethrow (false) ===");
    Promise<String> p5 = testConditional(false);
    String result5 = await p5;
    print("Result: " + result5.toString());

    print("");
    print("=== Test 6: Nested rethrow ===");
    Promise<String> p6 = nestedRethrow();
    String result6 = await p6;
    print("Result: " + result6.toString());

    print("");
    print("=== Test 7: Multiple rethrows through chain ===");
    Promise<String> p7 = level4Catch();
    String result7 = await p7;
    print("Result: " + result7.toString());

    print("");
    print("Rethrow test completed");
}

main();
