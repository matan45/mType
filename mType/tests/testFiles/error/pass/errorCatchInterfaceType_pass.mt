// Test: Catching exception that implements an interface (valid)
import * from "../../lib/exceptions/Exception.mt";

interface ILoggable {
    function log(): void;
}

class LoggableException extends Exception implements ILoggable {
    public constructor(string msg) : super(msg) {}

    public function log(): void {
        print("Logging exception: " + message);
    }

    public function toString(): string {
        return "LoggableException: " + message;
    }
}

class DetailedException extends LoggableException {
    public string details;

    public constructor(string msg, string det) : super(msg) {
        details = det;
    }

    public function toString(): string {
        return "DetailedException: " + message + " - " + details;
    }
}

function testInterfaceException(int testCase): void {
    try {
        if (testCase == 1) {
            throw new LoggableException("Loggable error");
        } else {
            throw new DetailedException("Detailed error", "Extra info");
        }
    } catch (LoggableException le) {
        le.log();
        print("Caught: " + le.getMessage());
    }
}

print("Testing interface-implementing exceptions");
testInterfaceException(1);
testInterfaceException(2);
print("Test completed");
