// Test exception inheritance hierarchy and catch polymorphism
import "../../lib/exceptions/Exception.mt";

// Create a hierarchy of custom exceptions
class ApplicationException extends Exception {
    constructor(string msg): super(msg) {
    }
}

class ValidationException extends ApplicationException {
    public string fieldName;

    constructor(string msg, string field): super(msg) {
        this.fieldName = field;
    }

    public function getFieldName(): string {
        return this.fieldName;
    }
}

class DatabaseException extends ApplicationException {
    public int errorCode;

    constructor(string msg, int code): super(msg) {
        this.errorCode = code;
    }

    public function getErrorCode(): int {
        return this.errorCode;
    }
}

class NullPointerException extends Exception {
    constructor(string msg): super(msg) {
    }
}

// Test 1: Catch base class catches derived class
function testCatchBaseClass(): string {
    try {
        throw new ValidationException("Invalid email", "email");
    } catch (ApplicationException e) {
        // Should catch ValidationException because it extends ApplicationException
        return "Caught ApplicationException: " + e.getMessage();
    } catch (Exception e) {
        return "Caught Exception (should not reach here)";
    }
    return "No exception caught";
}

// Test 2: Catch most specific type first
function testCatchSpecificFirst(): string {
    try {
        throw new DatabaseException("Connection failed", 1234);
    } catch (DatabaseException e) {
        // Should catch here (most specific)
        return "Caught DatabaseException with code: " + e.getErrorCode();
    } catch (ApplicationException e) {
        return "Caught ApplicationException (should not reach here)";
    } catch (Exception e) {
        return "Caught Exception (should not reach here)";
    }
    return "No exception caught";
}

// Test 3: Catch generic Exception catches all
function testCatchGenericException(): string {
    try {
        throw new NullPointerException("Object is null");
    } catch (Exception e) {
        // Should catch any exception type
        return "Caught Exception: " + e.getMessage();
    }
    return "No exception caught";
}

// Test 4: Wrong catch type doesn't match
function testWrongCatchType(): string {
    try {
        try {
            throw new ValidationException("Bad input", "username");
        } catch (DatabaseException e) {
            // Should NOT catch ValidationException
            return "ERROR: Caught wrong type";
        }
    } catch (Exception e) {
        // Outer catch should get it
        return "Outer catch got: " + e.getMessage();
    }
    return "No exception caught";
}

// Test 5: Multiple inheritance levels
function testMultipleLevels(): string {
    try {
        ValidationException e = new ValidationException("Field required", "name");
        throw e;
    } catch (Exception e) {
        // Should catch ValidationException (2 levels up: ValidationException -> ApplicationException -> Exception)
        return "Caught Exception (2 levels up): " + e.getMessage();
    }
    return "No exception caught";
}

// Test 6: Re-throw preserves type
function testRethrowPreservesType(): string {
    try {
        try {
            DatabaseException e = new DatabaseException("Timeout", 5678);
            throw e;
        } catch (ApplicationException e) {
            print("Inner catch: re-throwing");
            throw e;  // Re-throw as ApplicationException
        }
    } catch (DatabaseException e) {
        // Should still be able to catch as DatabaseException after re-throw
        return "Re-thrown exception still DatabaseException: " + e.getErrorCode();
    } catch (Exception e) {
        return "ERROR: Caught as generic Exception";
    }
    return "No exception caught";
}

// Run all tests
print("=== Test 1: Catch base class catches derived class ===");
print(testCatchBaseClass());

print("\n=== Test 2: Catch most specific type first ===");
print(testCatchSpecificFirst());

print("\n=== Test 3: Catch generic Exception catches all ===");
print(testCatchGenericException());

print("\n=== Test 4: Wrong catch type doesn't match ===");
print(testWrongCatchType());

print("\n=== Test 5: Multiple inheritance levels ===");
print(testMultipleLevels());

print("\n=== Test 6: Re-throw preserves type ===");
print(testRethrowPreservesType());

print("\nAll exception inheritance tests completed");
