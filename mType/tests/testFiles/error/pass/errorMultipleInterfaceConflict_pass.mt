// Test: Multiple interfaces with conflicting throws clauses
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/exceptions/IllegalArgumentException.mt";
import * from "../../lib/primitives/String.mt";

// First interface declares general validation
interface Validator {
    public function validate(string input): bool;
}

// Second interface declares processing
interface Processor {
    public function validate(string input): bool;
}

// Implementation satisfies both interfaces
class DataHandler implements Validator, Processor {
    public function validate(string input): bool {
        print("Validating: " + input);
        if (input == "exception") {
            throw new Exception("General exception thrown");
        }
        if (input == "runtime") {
            throw new RuntimeException("Runtime exception thrown");
        }
        return true;
    }
}

// Interface for strict validation
interface StrictValidator {
    public function validate(string input): bool;
}

// Implementation combining multiple interfaces
class CombinedHandler implements Validator, StrictValidator {
    public function validate(string input): bool {
        print("Combined validation: " + input);
        if (input == "illegal") {
            throw new IllegalArgumentException("Illegal argument");
        }
        if (input == "general") {
            throw new Exception("General error");
        }
        return true;
    }
}

// Three interfaces for different services
interface ServiceA {
    public function execute(string cmd): string;
}

interface ServiceB {
    public function execute(string cmd): string;
}

interface ServiceC {
    public function execute(string cmd): string;
}

// Implementation satisfies all three interfaces
class MultiService implements ServiceA, ServiceB, ServiceC {
    public function execute(string cmd): string {
        print("Multi-service executing: " + cmd);
        if (cmd == "illegal") {
            throw new IllegalArgumentException("Illegal command");
        }
        if (cmd == "runtime") {
            throw new RuntimeException("Runtime error");
        }
        if (cmd == "general") {
            throw new Exception("General error");
        }
        return "Success: " + cmd;
    }
}

function testDualInterface(): void {
    print("Testing dual interface implementation:");

    DataHandler handler = new DataHandler();

    try {
        bool result = handler.validate("valid");
        print("Validation passed: " + result);
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    }

    try {
        handler.validate("runtime");
        print("Should not reach here");
    } catch (RuntimeException e) {
        print("Caught runtime exception: " + e.getMessage());
    }

    try {
        handler.validate("exception");
        print("Should not reach here");
    } catch (Exception e) {
        print("Caught general exception: " + e.getMessage());
    }
}

function testCombinedHandler(): void {
    print("Testing combined handler:");

    CombinedHandler combined = new CombinedHandler();

    try {
        bool result = combined.validate("ok");
        print("Combined validation passed: " + result);
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    }

    try {
        combined.validate("illegal");
        print("Should not reach here");
    } catch (IllegalArgumentException e) {
        print("Caught illegal argument: " + e.getMessage());
    }

    try {
        combined.validate("general");
        print("Should not reach here");
    } catch (Exception e) {
        print("Caught general error: " + e.getMessage());
    }
}

function testMultiService(): void {
    print("Testing three-interface service:");

    MultiService service = new MultiService();

    try {
        string result = service.execute("normal");
        print("Execution result: " + result);
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    }

    try {
        service.execute("illegal");
        print("Should not reach here");
    } catch (IllegalArgumentException e) {
        print("Caught illegal: " + e.getMessage());
    }

    try {
        service.execute("runtime");
        print("Should not reach here");
    } catch (RuntimeException e) {
        print("Caught runtime: " + e.getMessage());
    }

    try {
        service.execute("general");
        print("Should not reach here");
    } catch (Exception e) {
        print("Caught general: " + e.getMessage());
    }
}

print("=== Test 1: Dual Interface ===");
testDualInterface();

print("");
print("=== Test 2: Combined Handler ===");
testCombinedHandler();

print("");
print("=== Test 3: Multi-Service ===");
testMultiService();

print("");
print("Multiple interface conflict test passed!");
