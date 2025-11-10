// Test: Multiple catch handlers in async functions
// Expected: Different exception types are caught by appropriate handlers
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

class ValidationException extends Exception {
    public constructor(string msg): super(msg) {
    }
}

class NetworkException extends RuntimeException {
    public constructor(string msg):super(msg) {
    }
}

class DatabaseException extends Exception {
    public constructor(string msg): super(msg) {
    }
}

// Test multiple catch blocks with different exception types
function async multipleCatchTypes(Int errorType): Promise<String> {
    try {
        print("Executing operation with type: " + errorType.toString());

        if (errorType.getValue() == 1) {
            ValidationException e = new ValidationException("Validation failed");
            throw e;
        }

        if (errorType.getValue() == 2) {
            NetworkException e = new NetworkException("Network error");
            throw e;
        }

        if (errorType.getValue() == 3) {
            DatabaseException e = new DatabaseException("Database error");
            throw e;
        }

        return new String("success");
    } catch (ValidationException e) {
        print("Caught ValidationException: " + e.getMessage());
        return new String("validation_error");
    } catch (NetworkException e) {
        print("Caught NetworkException: " + e.getMessage());
        return new String("network_error");
    } catch (DatabaseException e) {
        print("Caught DatabaseException: " + e.getMessage());
        return new String("database_error");
    } catch (Exception e) {
        print("Caught generic Exception: " + e.getMessage());
        return new String("generic_error");
    }
}

// Test catch order - most specific first
function async catchOrderTest(): Promise<String> {
    try {
        print("Testing catch order");
        RuntimeException e = new RuntimeException("Runtime issue");
        throw e;
        return new String("ok");
    } catch (RuntimeException e) {
        print("Caught RuntimeException (specific): " + e.getMessage());
        return new String("runtime_caught");
    } catch (Exception e) {
        print("Caught Exception (general): " + e.getMessage());
        return new String("exception_caught");
    }
}

// Test nested exceptions with multiple catch blocks
function async nestedMultipleCatch(): Promise<String> {
    try {
        print("Outer try");
        try {
            print("Inner try");
            ValidationException e = new ValidationException("Inner validation");
            throw e;
        } catch (NetworkException e) {
            print("Inner catch (network): " + e.getMessage());
            return new String("inner_network");
        } catch (ValidationException e) {
            print("Inner catch (validation): " + e.getMessage());
            DatabaseException de = new DatabaseException("Converted to database error");
            throw de;
        }
    } catch (DatabaseException e) {
        print("Outer catch (database): " + e.getMessage());
        return new String("outer_database");
    } catch (Exception e) {
        print("Outer catch (generic): " + e.getMessage());
        return new String("outer_generic");
    }
}

// Test exception inheritance hierarchy
class BaseError extends Exception {
    public constructor(string msg): super(msg) {
    }
}

class SpecificError extends BaseError {
    public constructor(string msg): super(msg) {
    }
}

function async inheritanceTest(bool useSpecific): Promise<String> {
    try {
        print("Testing inheritance");
        if (useSpecific) {
            SpecificError e = new SpecificError("Specific error");
            throw e;
        }
        BaseError e = new BaseError("Base error");
        throw e;
        return new String("ok");
    } catch (SpecificError e) {
        print("Caught SpecificError: " + e.getMessage());
        return new String("specific");
    } catch (BaseError e) {
        print("Caught BaseError: " + e.getMessage());
        return new String("base");
    }
}

function async main(): Promise<void> {
    print("=== Test 1: Multiple catch - Validation ===");
    Promise<String> p1 = multipleCatchTypes(new Int(1));
    String result1 = await p1;
    print("Result: " + result1.toString());

    print("");
    print("=== Test 2: Multiple catch - Network ===");
    Promise<String> p2 = multipleCatchTypes(new Int(2));
    String result2 = await p2;
    print("Result: " + result2.toString());

    print("");
    print("=== Test 3: Multiple catch - Database ===");
    Promise<String> p3 = multipleCatchTypes(new Int(3));
    String result3 = await p3;
    print("Result: " + result3.toString());

    print("");
    print("=== Test 4: Multiple catch - Success ===");
    Promise<String> p4 = multipleCatchTypes(new Int(0));
    String result4 = await p4;
    print("Result: " + result4.toString());

    print("");
    print("=== Test 5: Catch order ===");
    Promise<String> p5 = catchOrderTest();
    String result5 = await p5;
    print("Result: " + result5.toString());

    print("");
    print("=== Test 6: Nested multiple catch ===");
    Promise<String> p6 = nestedMultipleCatch();
    String result6 = await p6;
    print("Result: " + result6.toString());

    print("");
    print("=== Test 7: Inheritance - Specific ===");
    Promise<String> p7 = inheritanceTest(true);
    String result7 = await p7;
    print("Result: " + result7.toString());

    print("");
    print("=== Test 8: Inheritance - Base ===");
    Promise<String> p8 = inheritanceTest(false);
    String result8 = await p8;
    print("Result: " + result8.toString());

    print("");
    print("Multiple catch test completed");
    return null;
}

main();
