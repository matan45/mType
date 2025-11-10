// Test: @Throw annotation with polymorphic exception handling
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";

// Exception hierarchy
class ServiceException extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

class AuthenticationException extends ServiceException {
    public constructor(string msg) : super(msg) {
    }
}

class AuthorizationException extends ServiceException {
    public constructor(string msg) : super(msg) {
    }
}

class ConnectionException extends ServiceException {
    public constructor(string msg) : super(msg) {
    }
}

// Function declaring base exception type
@Throw(ServiceException)
function performServiceOperation(string operation, string user): string {
    print("Performing operation: " + operation + " for user: " + user);

    if (user == "unauthenticated") {
        // Can throw more specific exception (subtype of ServiceException)
        throw new AuthenticationException("User not authenticated");
    }

    if (user == "unauthorized") {
        // Can throw another specific exception (subtype of ServiceException)
        throw new AuthorizationException("User not authorized for this operation");
    }

    if (operation == "network-operation") {
        // Can throw another specific exception (subtype of ServiceException)
        throw new ConnectionException("Connection to service failed");
    }

    return "Operation completed successfully";
}

// Test case 1: Successful operation
function testSuccessfulOperation(): void {
    try {
        string result = performServiceOperation("read", "admin");
        print("Result: " + result);
    } catch (ServiceException e) {
        print("Unexpected error: " + e.getMessage());
    }
}

// Test case 2: Authentication error - catch specific type
function testAuthenticationError(): void {
    try {
        string result = performServiceOperation("read", "unauthenticated");
        print("Should not reach here");
    } catch (AuthenticationException e) {
        print("Caught specific AuthenticationException: " + e.getMessage());
    }
}

// Test case 3: Authorization error - catch base type
function testAuthorizationErrorBase(): void {
    try {
        string result = performServiceOperation("write", "unauthorized");
        print("Should not reach here");
    } catch (ServiceException e) {
        print("Caught base ServiceException: " + e.getMessage());
    }
}

// Test case 4: Connection error - catch with multiple handlers
function testConnectionErrorMultiple(): void {
    try {
        string result = performServiceOperation("network-operation", "admin");
        print("Should not reach here");
    } catch (AuthenticationException e) {
        print("Authentication error: " + e.getMessage());
    } catch (AuthorizationException e) {
        print("Authorization error: " + e.getMessage());
    } catch (ConnectionException e) {
        print("Caught ConnectionException: " + e.getMessage());
    } catch (ServiceException e) {
        print("Other service error: " + e.getMessage());
    }
}

// Test case 5: Using polymorphism with base exception handler
function testPolymorphicHandler(): void {
    string[] operations = ["read", "write", "network-operation"];
    string[] users = ["admin", "unauthenticated", "unauthorized"];

    for (int i = 0; i < 3; i = i + 1) {
        try {
            string result = performServiceOperation(operations[i], users[i]);
            print("Success: " + result);
        } catch (ServiceException e) {
            print("Polymorphic catch for operation " + operations[i] + ": " + e.getMessage());
        }
    }
}

print("=== Test 1: Successful operation ===");
testSuccessfulOperation();

print("");
print("=== Test 2: Authentication error ===");
testAuthenticationError();

print("");
print("=== Test 3: Authorization error (base catch) ===");
testAuthorizationErrorBase();

print("");
print("=== Test 4: Connection error (multiple handlers) ===");
testConnectionErrorMultiple();

print("");
print("=== Test 5: Polymorphic handler ===");
testPolymorphicHandler();

print("");
print("Polymorphic exception test passed!");
