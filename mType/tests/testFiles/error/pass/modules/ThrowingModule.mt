// Module that throws an exception during initialization
import * from "../../../lib/exceptions/RuntimeException.mt";

class InitializationException extends RuntimeException {
    public constructor(string msg) : super(msg) {
    }

    public function toString(): string {
        return "InitializationException: " + message;
    }
}

// This will throw during module initialization
if (true) {
    throw new InitializationException("Module initialization failed");
}

class ModuleClass {
    public function test(): string {
        return "This should never be reached";
    }
}
