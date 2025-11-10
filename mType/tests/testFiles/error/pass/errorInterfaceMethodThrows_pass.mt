// Test: Interface method declaring throws clause
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";

// Interface with method signatures
interface DataValidator {
    public function validate(string data): bool;
    public function process(string data): string;
}

// Implementation that may throw exceptions
class InputValidator implements DataValidator {
    public function validate(string data): bool {
        print("Validating: " + data);
        if (data == "invalid") {
            throw new Exception("Invalid data detected");
        }
        return true;
    }

    public function process(string data): string {
        print("Processing: " + data);
        if (data == "error") {
            throw new RuntimeException("Processing error occurred");
        }
        return "Processed: " + data;
    }
}

function testValidData(): void {
    InputValidator validator = new InputValidator();

    try {
        bool isValid = validator.validate("valid");
        print("Validation result: " + isValid);

        string result = validator.process("data");
        print("Process result: " + result);
    } catch (Exception e) {
        print("Unexpected exception: " + e.getMessage());
    }
}

function testInvalidData(): void {
    InputValidator validator = new InputValidator();

    try {
        bool isValid = validator.validate("invalid");
        print("Should not reach here");
    } catch (Exception e) {
        print("Caught validation exception: " + e.getMessage());
    }
}

function testProcessError(): void {
    InputValidator validator = new InputValidator();

    try {
        string result = validator.process("error");
        print("Should not reach here");
    } catch (RuntimeException e) {
        print("Caught processing exception: " + e.getMessage());
    }
}

print("=== Test 1: Valid data ===");
testValidData();

print("");
print("=== Test 2: Invalid data ===");
testInvalidData();

print("");
print("=== Test 3: Process error ===");
testProcessError();

print("");
print("Interface method throws test passed!");
