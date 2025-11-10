// Test: Generic method throwing exception
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/int.mt";
import * from "../../lib/primitives/string.mt";

// Generic exception for method errors
class MethodException<T> extends Exception {
    public T returnValue;
    public bool hasReturnValue;

    public constructor(string msg): super(msg) {
        hasReturnValue = false;
    }

    public constructor(string msg, T value): super(msg) {
        returnValue = value;
        hasReturnValue = true;
    }

    public function getReturnValue(): T {
        return returnValue;
    }
}

// Class with generic methods that throw exceptions
class Calculator {
    // Generic method that may throw
    public static function <T> validateAndProcess(T value, function(T): bool validator): T {
        if (!validator(value)) {
            throw new MethodException<T>("Validation failed", value);
        }
        return value;
    }

    // Generic method with type-specific validation
    public function <T> safeConvert(T input, function(T): T converter): T {
        try {
            return converter(input);
        } catch (Exception e) {
            throw new MethodException<T>("Conversion failed for input");
        }
    }
}

// Validator functions
function isPositiveInt(int value): bool {
    return value > new int(0);
}

function isNonEmptyString(string value): bool {
    return value != new string("");
}

// Converter functions
function doubleInt(int value): int {
    return new int(value * new int(2));
}

function upperString(string value): string {
    return value;  // Simplified - would normally uppercase
}

// Test generic method throwing int exception
print("Testing generic method with int:");
try {
    int result = Calculator.validateAndProcess<int>(new int(-5), isPositiveInt);
} catch (MethodException<int> e) {
    print("Caught int method exception: " + e.getMessage());
    print("Invalid value: " + e.getReturnValue());
}

// Test generic method throwing string exception
print("Testing generic method with string:");
try {
    string result = Calculator.validateAndProcess<string>(new string(""), isNonEmptyString);
} catch (MethodException<string> e) {
    print("Caught string method exception: " + e.getMessage());
    print("Invalid value: [" + e.getReturnValue() + "]");
}

// Test successful case
print("Testing generic method success:");
try {
    int valid = Calculator.validateAndProcess<int>(new int(10), isPositiveInt);
    print("Validated value: " + valid);
} catch (MethodException<int> e) {
    print("Should not reach here");
}

// Test instance generic method
print("Testing instance generic method:");
Calculator calc = new Calculator();
try {
    int doubled = calc.safeConvert<int>(new int(5), doubleInt);
    print("Converted value: " + doubled);
} catch (MethodException<int> e) {
    print("Conversion error: " + e.getMessage());
}

print("Generic method exception test passed!");
