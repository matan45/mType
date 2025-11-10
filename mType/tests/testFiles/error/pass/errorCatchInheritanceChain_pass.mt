// Test: Complex exception inheritance hierarchy with proper catch ordering
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";

class DatabaseException extends RuntimeException {
    public constructor(string msg) : super(msg) {}

    public function toString(): string {
        return "DatabaseException: " + message;
    }
}

class ConnectionException extends DatabaseException {
    public constructor(string msg) : super(msg) {}

    public function toString(): string {
        return "ConnectionException: " + message;
    }
}

class TimeoutException extends ConnectionException {
    public constructor(string msg) : super(msg) {}

    public function toString(): string {
        return "TimeoutException: " + message;
    }
}

class QueryException extends DatabaseException {
    public constructor(string msg) : super(msg) {}

    public function toString(): string {
        return "QueryException: " + message;
    }
}

class ValidationException extends RuntimeException {
    public constructor(string msg) : super(msg) {}

    public function toString(): string {
        return "ValidationException: " + message;
    }
}

function testComplexHierarchy(int testCase): void {
    try {
        if (testCase == 1) {
            throw new TimeoutException("Connection timed out");
        } else if (testCase == 2) {
            throw new ConnectionException("Cannot connect");
        } else if (testCase == 3) {
            throw new QueryException("Invalid query");
        } else if (testCase == 4) {
            throw new ValidationException("Validation failed");
        } else {
            throw new DatabaseException("Database error");
        }
    } catch (TimeoutException te) {
        // Most specific first
        print("Caught TimeoutException: " + te.getMessage());
    } catch (ConnectionException ce) {
        // More general than Timeout
        print("Caught ConnectionException: " + ce.getMessage());
    } catch (QueryException qe) {
        // Sibling of Connection, also extends Database
        print("Caught QueryException: " + qe.getMessage());
    } catch (DatabaseException de) {
        // Parent of Connection and Query
        print("Caught DatabaseException: " + de.getMessage());
    } catch (ValidationException ve) {
        // Unrelated to Database hierarchy, same level as Database
        print("Caught ValidationException: " + ve.getMessage());
    } catch (RuntimeException re) {
        // Parent of Database and Validation
        print("Caught RuntimeException: " + re.getMessage());
    } catch (Exception e) {
        // Top-level catch-all
        print("Caught Exception: " + e.getMessage());
    }
}

print("Testing complex inheritance hierarchy");
testComplexHierarchy(1);
testComplexHierarchy(2);
testComplexHierarchy(3);
testComplexHierarchy(4);
testComplexHierarchy(5);
print("Test completed");
