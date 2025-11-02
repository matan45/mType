// Test: Generic method throwing exception
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic exception for method errors
class MethodException<T> extends Exception {
    public T returnValue;
    public bool hasReturnValue;

    public constructor(string msg) {
        super(msg);
        hasReturnValue = false;
    }

    public constructor(string msg, T value) {
        super(msg);
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
function isPositiveInt(Int value): bool {
    return value > new Int(0);
}

function isNonEmptyString(String value): bool {
    return value != new String("");
}

// Converter functions
function doubleInt(Int value): Int {
    return new Int(value * new Int(2));
}

function upperString(String value): String {
    return value;  // Simplified - would normally uppercase
}

// Test generic method throwing Int exception
print("Testing generic method with Int:");
try {
    Int result = Calculator.validateAndProcess<Int>(new Int(-5), isPositiveInt);
} catch (MethodException<Int> e) {
    print("Caught Int method exception: " + e.getMessage());
    print("Invalid value: " + e.getReturnValue());
}

// Test generic method throwing String exception
print("Testing generic method with String:");
try {
    String result = Calculator.validateAndProcess<String>(new String(""), isNonEmptyString);
} catch (MethodException<String> e) {
    print("Caught String method exception: " + e.getMessage());
    print("Invalid value: [" + e.getReturnValue() + "]");
}

// Test successful case
print("Testing generic method success:");
try {
    Int valid = Calculator.validateAndProcess<Int>(new Int(10), isPositiveInt);
    print("Validated value: " + valid);
} catch (MethodException<Int> e) {
    print("Should not reach here");
}

// Test instance generic method
print("Testing instance generic method:");
Calculator calc = new Calculator();
try {
    Int doubled = calc.safeConvert<Int>(new Int(5), doubleInt);
    print("Converted value: " + doubled);
} catch (MethodException<Int> e) {
    print("Conversion error: " + e.getMessage());
}

print("Generic method exception test passed!");
