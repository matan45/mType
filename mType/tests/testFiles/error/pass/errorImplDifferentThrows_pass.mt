// Test: Implementation with different throws clause than interface
// Expected: Should compile and run successfully (implementation can declare more specific exceptions)
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/exceptions/IllegalArgumentException.mt";
import * from "../../lib/primitives/String.mt";

// Interface declares general Exception
interface DataProcessor {
    function process(string input) throws Exception: string;
}

// Implementation declares more specific RuntimeException
class SpecificProcessor implements DataProcessor {
    public function process(string input) throws RuntimeException: string {
        print("Processing with RuntimeException: " + input);
        if (input == "runtime_error") {
            throw new RuntimeException("Specific runtime error");
        }
        return "Result: " + input;
    }
}

// Implementation declares even more specific IllegalArgumentException
class VerySpecificProcessor implements DataProcessor {
    public function process(string input) throws IllegalArgumentException: string {
        print("Processing with IllegalArgumentException: " + input);
        if (input == "illegal") {
            throw new IllegalArgumentException("Illegal argument detected");
        }
        return "Valid: " + input;
    }
}

// Implementation declares no throws (doesn't throw any exception)
class SafeProcessor implements DataProcessor {
    public function process(string input): string {
        print("Safe processing: " + input);
        return "Safe result: " + input;
    }
}

function testSpecificProcessor(): void {
    SpecificProcessor processor = new SpecificProcessor();

    try {
        string result = processor.process("test");
        print("Success: " + result);
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    }

    try {
        string result = processor.process("runtime_error");
        print("Should not reach here");
    } catch (RuntimeException e) {
        print("Caught runtime exception: " + e.getMessage());
    }
}

function testVerySpecificProcessor(): void {
    VerySpecificProcessor processor = new VerySpecificProcessor();

    try {
        string result = processor.process("valid_input");
        print("Success: " + result);
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    }

    try {
        string result = processor.process("illegal");
        print("Should not reach here");
    } catch (IllegalArgumentException e) {
        print("Caught illegal argument exception: " + e.getMessage());
    }
}

function testSafeProcessor(): void {
    SafeProcessor processor = new SafeProcessor();

    string result = processor.process("always_safe");
    print("Safe result: " + result);
}

print("=== Test 1: Specific RuntimeException ===");
testSpecificProcessor();

print("");
print("=== Test 2: Very Specific IllegalArgumentException ===");
testVerySpecificProcessor();

print("");
print("=== Test 3: No throws clause ===");
testSafeProcessor();

print("");
print("Implementation different throws test passed!");
