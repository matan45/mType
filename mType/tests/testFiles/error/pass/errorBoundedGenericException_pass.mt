// Test: Bounded generic exception types
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/int.mt";
import * from "../../lib/primitives/string.mt";

// Base error info interface
interface ErrorInfo {
    public function getErrorCode(): string;
}

// Specific error info implementation
class ValidationErrorInfo implements ErrorInfo {
    private string code;
    private string field;

    public constructor(string errorCode, string fieldName) {
        code = errorCode;
        field = fieldName;
    }

    public function getErrorCode(): string {
        return code;
    }

    public function getField(): string {
        return field;
    }
}

// Generic exception bounded by ErrorInfo interface
class BoundedException<T extends ErrorInfo> extends Exception {
    public T errorInfo;

    public constructor(string msg, T info): super(msg) {
        errorInfo = info;
    }

    public function getErrorInfo(): T {
        return errorInfo;
    }

    public function getErrorCode(): string {
        return errorInfo.getErrorCode();
    }
}

// Test bounded generic exception
function testBoundedException(): void {
    try {
        ValidationErrorInfo validationInfo = new ValidationErrorInfo("REQUIRED_FIELD", "username");
        throw new BoundedException<ValidationErrorInfo>("Validation failed", validationInfo);
    } catch (BoundedException<ValidationErrorInfo> e) {
        print("Caught bounded exception: " + e.getMessage());
        print("Error code: " + e.getErrorCode());
        print("Field: " + e.getErrorInfo().getField());
    }
}

print("Testing bounded generic exception:");
testBoundedException();
print("Bounded generic exception test passed!");
