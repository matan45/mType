// Test: @Throw with a parameter name other than 'exceptions' — should fail.
// AnnotationUsageValidator rejects unknown parameter names with the message
// "Unknown parameter '<name>' for annotation '@Throw'.".
import * from "../../lib/exceptions/Exception.mt";

class MyException extends Exception {
    constructor(string message): super(message) {
    }
}

@Throw(types = [MyException])
function doWork(): void {
}

// This should never execute due to validation error
doWork();
