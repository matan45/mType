// Custom exception type for cross-module testing
import * from "../../../lib/exceptions/RuntimeException.mt";

class CustomException extends RuntimeException {
    public constructor(string msg) : super(msg) {
    }

    public function toString(): string {
        return "CustomException: " + message;
    }
}
