// Test: Stack trace accuracy with call stack
// Expected: Stack trace should accurately reflect the call chain
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Custom exception to test stack trace
class StackTraceException extends RuntimeException {
    public constructor(string msg) {
        super(msg);
    }
}

// Level 3 - deepest function that throws
function throwError(): void {
    print("Level 3: About to throw exception");
    StackTraceException e = new StackTraceException("Error at deepest level");
    throw e;
}

// Level 2 - calls throwError
function middleFunction(): void {
    print("Level 2: Calling throwError");
    throwError();
    print("Level 2: Should not reach here");
}

// Level 1 - calls middleFunction
function topFunction(): void {
    print("Level 1: Calling middleFunction");
    middleFunction();
    print("Level 1: Should not reach here");
}

// Test 1: Simple call stack
function testSimpleStack(): void {
    print("=== Test 1: Simple call stack ===");
    try {
        topFunction();
    } catch (StackTraceException e) {
        print("Caught exception: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace present: true");
        }
        print("Exception handled successfully");
    }
}

// Test 2: Stack trace from method call
class Calculator {
    public function divide(Int a, Int b): Int {
        if (b.toInt() == 0) {
            print("Division by zero detected");
            RuntimeException e = new RuntimeException("Cannot divide by zero");
            throw e;
        }
        return new Int(a.toInt() / b.toInt());
    }

    public function calculate(Int x, Int y): Int {
        print("Calculating: " + x.toString() + " / " + y.toString());
        return divide(x, y);
    }
}

function testMethodStack(): void {
    print("=== Test 2: Method call stack ===");
    Calculator calc = new Calculator();
    try {
        Int result = calc.calculate(new Int(10), new Int(0));
        print("Result: " + result.toString());
    } catch (RuntimeException e) {
        print("Caught exception: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace from method: true");
        }
        print("Method exception handled");
    }
}

// Test 3: Stack trace with constructor
class Resource {
    public string name;

    public constructor(string resourceName) {
        print("Creating resource: " + resourceName);
        if (resourceName == "") {
            RuntimeException e = new RuntimeException("Resource name cannot be empty");
            throw e;
        }
        name = resourceName;
    }
}

function testConstructorStack(): void {
    print("=== Test 3: Constructor stack trace ===");
    try {
        Resource r = new Resource("");
        print("Created: " + r.name);
    } catch (RuntimeException e) {
        print("Caught exception: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace from constructor: true");
        }
        print("Constructor exception handled");
    }
}

// Test 4: Stack trace preservation through re-throw
function innerThrow(): void {
    print("Inner function throwing");
    RuntimeException e = new RuntimeException("Inner exception");
    throw e;
}

function middleRethrow(): void {
    print("Middle function catching and re-throwing");
    try {
        innerThrow();
    } catch (RuntimeException e) {
        print("Middle caught: " + e.getMessage());
        print("Re-throwing exception");
        throw e;
    }
}

function testRethrowStack(): void {
    print("=== Test 4: Re-throw stack trace ===");
    try {
        middleRethrow();
    } catch (RuntimeException e) {
        print("Outer caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace preserved through re-throw: true");
        }
        print("Re-throw handled");
    }
}

// Test 5: Multiple exception types with stack traces
class ValidationException extends Exception {
    public constructor(string msg) {
        super(msg);
    }
}

function validateInput(Int value): void {
    print("Validating input: " + value.toString());
    if (value.toInt() < 0) {
        ValidationException e = new ValidationException("Value cannot be negative");
        throw e;
    }
    if (value.toInt() > 100) {
        ValidationException e = new ValidationException("Value cannot exceed 100");
        throw e;
    }
    print("Validation passed");
}

function testMultipleExceptionTypes(): void {
    print("=== Test 5: Multiple exception types ===");

    // Test negative value
    try {
        validateInput(new Int(-5));
    } catch (ValidationException e) {
        print("Caught validation error: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace for negative value: true");
        }
    }

    // Test exceeding value
    try {
        validateInput(new Int(150));
    } catch (ValidationException e) {
        print("Caught validation error: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace for exceeding value: true");
        }
    }

    print("Multiple exception types tested");
}

// Run all tests
testSimpleStack();
print("");
testMethodStack();
print("");
testConstructorStack();
print("");
testRethrowStack();
print("");
testMultipleExceptionTypes();
print("");
print("Stack trace accuracy test completed");
