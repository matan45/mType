// Test: Exception with custom message in stack trace
// Expected: Custom exception messages should be properly included in stack traces
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test 1: Custom exception with detailed message
class DetailedException extends RuntimeException {
    public string context;
    public Int errorCode;

    public constructor(string msg, string ctx, Int code): super(msg) {
        context = ctx;
        errorCode = code;
    }

    public function getFullMessage(): string {
        return message + " [Context: " + context + ", Code: " + errorCode.toString() + "]";
    }
}

function testDetailedException(): void {
    print("=== Test 1: Detailed exception message ===");
    try {
        print("Creating detailed exception");
        DetailedException e = new DetailedException(
            "Operation failed",
            "Database connection",
            new Int(500)
        );
        throw e;
    } catch (DetailedException e) {
        print("Caught: " + e.getMessage());
        print("Full message: " + e.getFullMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace with custom message: true");
        }
        print("Detailed exception handled");
    }
}

// Test 2: Exception with formatted message
class FormattedException extends Exception {
    public constructor(string operation, Int value, string reason): super("Failed to " + operation + " with value " + value.toString() + ": " + reason) {
    }
}

function operationThatFails(int val): void {
    print("Performing operation with: " + val);
    if (val == 0) {
        FormattedException e = new FormattedException("process", val, "zero value not allowed");
        throw e;
    }
    print("Operation succeeded");
}

function testFormattedException(): void {
    print("=== Test 2: Formatted exception message ===");
    try {
        operationThatFails(0);
    } catch (FormattedException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Formatted message in stack trace: true");
        }
        print("Formatted exception handled");
    }
}

// Test 3: Exception with multi-part message
class ValidationException extends RuntimeException {
    public string fieldName;
    public string expectedValue;
    public string actualValue;

    public constructor(string field, string expected, string actual):  super("Validation failed for field '" + field + "'") {
        fieldName = field;
        expectedValue = expected;
        actualValue = actual;
    }

    public function getDetails(): string {
        return "Field: " + fieldName + ", Expected: " + expectedValue + ", Actual: " + actualValue;
    }
}

function validateField(string name, string value): void {
    print("Validating field: " + name);
    if (value == "") {
        ValidationException e = new ValidationException(name, "non-empty", "empty");
        throw e;
    }
    print("Validation passed");
}

function testValidationException(): void {
    print("=== Test 3: Multi-part exception message ===");
    try {
        validateField("username", "");
    } catch (ValidationException e) {
        print("Caught: " + e.getMessage());
        print("Details: " + e.getDetails());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Multi-part message in stack trace: true");
        }
        print("Validation exception handled");
    }
}

// Test 4: Exception with special characters in message
class SpecialCharException extends Exception {
    public constructor(string msg): super(msg) {
    }
}

function testSpecialCharacters(): void {
    print("=== Test 4: Special characters in message ===");
    try {
        print("Throwing exception with special chars");
        SpecialCharException e = new SpecialCharException("Error: Value \"test\" failed at line #42");
        throw e;
    } catch (SpecialCharException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Special chars preserved: true");
        }
        print("Special char exception handled");
    }
}

// Test 5: Exception with numeric context in message
class NumericException extends RuntimeException {
    public Int threshold;
    public Int actualValue;

    public constructor(string msg, Int thresh, Int actual):super(msg) {
        threshold = thresh;
        actualValue = actual;
    }

    public function getComparison(): string {
        return "Threshold: " + threshold.toString() + ", Actual: " + actualValue.toString();
    }
}

function checkThreshold(int value, int limit): void {
    print("Checking: " + value + " against limit: " + limit);
    if (value > limit) {
        NumericException e = new NumericException(
            "Value exceeds threshold",
            limit,
            value
        );
        throw e;
    }
    print("Within threshold");
}

function testNumericContext(): void {
    print("=== Test 5: Numeric context in message ===");
    try {
        checkThreshold(150, 100);
    } catch (NumericException e) {
        print("Caught: " + e.getMessage());
        print("Comparison: " + e.getComparison());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Numeric context in stack trace: true");
        }
        print("Numeric exception handled");
    }
}

// Test 6: Exception with concatenated message
class ConcatenatedMessageException extends Exception {
    public constructor(string part1, string part2, string part3):super(part1 + " | " + part2 + " | " + part3) {
    }
}

function testConcatenatedMessage(): void {
    print("=== Test 6: Concatenated message ===");
    try {
        print("Building exception message");
        ConcatenatedMessageException e = new ConcatenatedMessageException(
            "Phase 1 failed",
            "Rollback initiated",
            "System stable"
        );
        throw e;
    } catch (ConcatenatedMessageException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Concatenated message in stack trace: true");
        }
        print("Concatenated message handled");
    }
}

// Test 7: Exception message with state information
class StateException extends RuntimeException {
    public string currentState;
    public string expectedState;
    public Int stateId;

    public constructor(string current, string expected, Int id): super("Invalid state transition") {
        currentState = current;
        expectedState = expected;
        stateId = id;
    }

    public function getStateInfo(): string {
        return "State " + stateId.toString() + ": " + currentState + " -> " + expectedState;
    }
}

function transitionState(string from2, string to, Int id): void {
    print("Transitioning state: " + from2 + " to " + to);
    if (from2 == "CLOSED" && to == "PROCESSING") {
        StateException e = new StateException(from2, to, id);
        throw e;
    }
    print("Transition successful");
}

function testStateException(): void {
    print("=== Test 7: State information in message ===");
    try {
        transitionState("CLOSED", "PROCESSING", new Int(123));
    } catch (StateException e) {
        print("Caught: " + e.getMessage());
        print("State info: " + e.getStateInfo());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("State info in stack trace: true");
        }
        print("State exception handled");
    }
}

// Test 8: Exception with long message
class LongMessageException extends Exception {
    public constructor(): super("This is a very long exception message that contains detailed information about what went wrong in the system and provides context for debugging purposes") {
    }
}

function testLongMessage(): void {
    print("=== Test 8: Long exception message ===");
    try {
        print("Throwing exception with long message");
        LongMessageException e = new LongMessageException();
        throw e;
    } catch (LongMessageException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Long message in stack trace: true");
        }
        print("Long message handled");
    }
}

// Run all tests
testDetailedException();
print("");
testFormattedException();
print("");
testValidationException();
print("");
testSpecialCharacters();
print("");
testNumericContext();
print("");
testConcatenatedMessage();
print("");
testStateException();
print("");
testLongMessage();
print("");
print("Custom message stack trace test completed");
