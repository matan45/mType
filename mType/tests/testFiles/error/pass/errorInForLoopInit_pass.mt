// Test exception handling in for-loop initializer
import * from "../../lib/exceptions/Exception.mt";

class InitException extends Exception {
    constructor(string message): super(message) {
    }
}

function computeInitValue(): int {
    try {
        print("Computing for-loop init value");
        throw new InitException("Error in for-loop initializer");
    } catch (InitException e) {
        print("Caught in init: " + e.getMessage());
        return 0;
    }
}

function main(): void {
    print("Testing exception in for-loop initializer");

    for (int i = computeInitValue(); i < 3; i = i + 1) {
        print("Loop iteration: " + i);
    }

    print("Test completed");
}

main();
