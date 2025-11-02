// Test: Exception thrown from lambda and propagated to caller
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";

interface Validator {
    function validate(int value): bool;
}

interface Transformer {
    function transform(string input): string;
}

function main(): void {
    print("Testing exception thrown from lambda");

    // Lambda that throws on invalid input
    Validator validator = x -> {
        if (x < 0) {
            throw new Exception("Negative value not allowed");
        }
        if (x > 1000) {
            throw new Exception("Value exceeds maximum");
        }
        return true;
    };

    // Test valid input
    try {
        bool result = validator.validate(42);
        print("Validation passed for 42: " + result);
    } catch (Exception e) {
        print("Unexpected exception: " + e.getMessage());
    }

    // Test negative input
    try {
        bool result = validator.validate(-5);
        print("Validation passed for -5: " + result);
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    }

    // Test large input
    try {
        bool result = validator.validate(2000);
        print("Validation passed for 2000: " + result);
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    }

    // String transformer lambda with exception
    Transformer transformer = s -> {
        if (s == null || s.length() == 0) {
            throw new Exception("Empty string not allowed");
        }
        return s.toUpperCase();
    };

    try {
        string result = transformer.transform("hello");
        print("Transformed: " + result);
    } catch (Exception e) {
        print("Transform error: " + e.getMessage());
    }

    try {
        string result = transformer.transform("");
        print("Transformed: " + result);
    } catch (Exception e) {
        print("Transform error: " + e.getMessage());
    }

    print("Lambda exception throwing test completed");
}

main();
