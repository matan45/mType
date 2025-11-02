// Test: Default interface method throwing exception
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Interface with default method that throws exception
interface Logger {
    function log(string message): void;

    // Default method with exception
    default function logError(string error) throws RuntimeException: void {
        print("Default logError called");
        if (error == "critical") {
            throw new RuntimeException("Critical error: " + error);
        }
        log("[ERROR] " + error);
    }

    // Default method that calls another default method
    default function logWithPrefix(string prefix, string message) throws Exception: void {
        print("Default logWithPrefix called");
        if (prefix == "FATAL") {
            throw new Exception("Fatal error with message: " + message);
        }
        log(prefix + ": " + message);
    }
}

// Implementation that uses default methods
class ConsoleLogger implements Logger {
    public function log(string message): void {
        print("Console: " + message);
    }
}

// Implementation that overrides default method with its own exception handling
class FileLogger implements Logger {
    public function log(string message): void {
        print("File: " + message);
    }

    public function logError(string error) throws RuntimeException: void {
        print("Custom logError called");
        if (error == "disk_full") {
            throw new RuntimeException("Disk full error: " + error);
        }
        log("[FILE_ERROR] " + error);
    }
}

// Implementation that overrides without throws
class SafeLogger implements Logger {
    public function log(string message): void {
        print("Safe: " + message);
    }

    public function logError(string error): void {
        print("Safe logError called (no exceptions)");
        log("[SAFE_ERROR] " + error);
    }
}

function testDefaultMethodException(): void {
    print("Testing default method with exception:");

    ConsoleLogger logger = new ConsoleLogger();

    try {
        logger.logError("minor");
        print("Minor error logged successfully");
    } catch (RuntimeException e) {
        print("Caught exception: " + e.getMessage());
    }

    try {
        logger.logError("critical");
        print("Should not reach here");
    } catch (RuntimeException e) {
        print("Caught critical exception: " + e.getMessage());
    }
}

function testDefaultMethodCalling(): void {
    print("Testing default method calling another method:");

    ConsoleLogger logger = new ConsoleLogger();

    try {
        logger.logWithPrefix("INFO", "System started");
        print("Prefix logged successfully");
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    }

    try {
        logger.logWithPrefix("FATAL", "System crash");
        print("Should not reach here");
    } catch (Exception e) {
        print("Caught fatal exception: " + e.getMessage());
    }
}

function testOverriddenDefault(): void {
    print("Testing overridden default method:");

    FileLogger fileLogger = new FileLogger();

    try {
        fileLogger.logError("warning");
        print("Warning logged to file");
    } catch (RuntimeException e) {
        print("Caught exception: " + e.getMessage());
    }

    try {
        fileLogger.logError("disk_full");
        print("Should not reach here");
    } catch (RuntimeException e) {
        print("Caught disk full exception: " + e.getMessage());
    }
}

function testSafeOverride(): void {
    print("Testing safe override (no exceptions):");

    SafeLogger safeLogger = new SafeLogger();
    safeLogger.logError("any_error");
    print("Safe error handling completed");
}

print("=== Test 1: Default Method Exception ===");
testDefaultMethodException();

print("");
print("=== Test 2: Default Method Calling ===");
testDefaultMethodCalling();

print("");
print("=== Test 3: Overridden Default ===");
testOverriddenDefault();

print("");
print("=== Test 4: Safe Override ===");
testSafeOverride();

print("");
print("Default method exception test passed!");
