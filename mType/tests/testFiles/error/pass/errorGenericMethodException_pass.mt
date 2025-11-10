// Test: Generic method throwing exception
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

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

// Generic interfaces for validators and converters
interface Validator<T> {
    function validate(T value): Bool;
}

interface Converter<T> {
    function convert(T value): T;
}

// Class with generic methods that throw exceptions
class Calculator {
    // Generic method that may throw
    public static function <T> validateAndProcess(T value, Validator<T> validator): T {
        if (!validator.validate(value).getValue()) {
            throw new MethodException<T>("Validation failed", value);
        }
        return value;
    }

    // Generic method with type-specific validation
    public function <T> safeConvert(T input, Converter<T> converter): T {
        try {
            return converter.convert(input);
        } catch (Exception e) {
            throw new MethodException<T>("Conversion failed for input");
        }
    }
}

// Test generic method throwing int exception
print("Testing generic method with int:");
try {
    Validator<Int> isPositiveInt = value -> value > new Int(0);
    int result = Calculator::validateAndProcess<Int>(new Int(-5), isPositiveInt);
} catch (MethodException<Int> e) {
    print("Caught int method exception: " + e.getMessage());
    print("Invalid value: " + e.getReturnValue());
}

// Test generic method throwing string exception
print("Testing generic method with string:");
try {
    Validator<String> isNonEmptyString = value -> value != new String("");
    string result = Calculator::validateAndProcess<String>(new String(""), isNonEmptyString);
} catch (MethodException<String> e) {
    print("Caught string method exception: " + e.getMessage());
    print("Invalid value: [" + e.getReturnValue() + "]");
}

// Test successful case
print("Testing generic method success:");
try {
    Validator<Int> isPositiveInt2 = value -> value > new Int(0);
    int valid = Calculator::validateAndProcess<Int>(new Int(10), isPositiveInt2);
    print("Validated value: " + valid);
} catch (MethodException<Int> e) {
    print("Should not reach here");
}

// Test instance generic method
print("Testing instance generic method:");
Calculator calc = new Calculator();
try {
    Converter<Int> doubleInt = value -> value * new Int(2);
    int doubled = calc.safeConvert<Int>(new Int(5), doubleInt);
    print("Converted value: " + doubled);
} catch (MethodException<Int> e) {
    print("Conversion error: " + e.getMessage());
}

print("Generic method exception test passed!");
