// Test: Try/Catch in standalone exe
import * from "../../lib/exceptions/Exception.mt";

class ValidationException extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

class NotFoundException extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

function riskyOperation(int code): string {
    if (code == 1) {
        throw new ValidationException("Invalid input");
    }
    if (code == 2) {
        throw new NotFoundException("Item not found");
    }
    return "Success with code " + code;
}

function nestedTryCatch(): void {
    try {
        try {
            throw new ValidationException("Inner error");
        } catch (ValidationException e) {
            print("Inner catch: " + e.getMessage());
            throw new NotFoundException("Converted to not found");
        }
    } catch (NotFoundException e) {
        print("Outer catch: " + e.getMessage());
    }
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Basic try/catch
        try {
            string result = riskyOperation(0);
            print(result);
        } catch (Exception e) {
            print("Unexpected error");
        }

        // Catch specific exception
        try {
            riskyOperation(1);
        } catch (ValidationException e) {
            print("Caught validation: " + e.getMessage());
        } catch (Exception e) {
            print("Caught generic: " + e.getMessage());
        }

        // Different exception type
        try {
            riskyOperation(2);
        } catch (ValidationException e) {
            print("Wrong catch");
        } catch (NotFoundException e) {
            print("Caught not found: " + e.getMessage());
        }

        // Nested try/catch
        nestedTryCatch();

        // Exception propagation
        try {
            riskyOperation(1);
        } catch (Exception e) {
            print("Propagated: " + e.getMessage());
        }

        print("Try/Catch test passed");
    }
}
