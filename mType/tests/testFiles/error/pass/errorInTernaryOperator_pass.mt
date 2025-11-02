// Test exception handling in ternary operator
import * from "../../lib/exceptions/Exception.mt";

class TernaryException extends Exception {
    constructor(String message) {
        super(message);
    }
}

function computeTrue(): Int {
    try {
        print("Computing true branch");
        throw new TernaryException("Error in true branch");
    } catch (TernaryException e) {
        print("Caught in true branch: " + e.getMessage());
        return 100;
    }
}

function computeFalse(): Int {
    try {
        print("Computing false branch");
        throw new TernaryException("Error in false branch");
    } catch (TernaryException e) {
        print("Caught in false branch: " + e.getMessage());
        return 200;
    }
}

function main(): void {
    print("Testing exception in ternary operator");

    Int result1 = true ? computeTrue() : 0;
    print("Result 1: " + result1);
    print("---");

    Int result2 = false ? 0 : computeFalse();
    print("Result 2: " + result2);

    print("Test completed");
}

main();
