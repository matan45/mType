// Test exception handling in for-loop condition
import * from "../../lib/exceptions/Exception.mt";

class ConditionException extends Exception {
    constructor(String message) {
        super(message);
    }
}

Int checkCount = 0;

function checkCondition(Int i): Bool {
    checkCount = checkCount + 1;
    try {
        print("Checking condition for i=" + i + " (check #" + checkCount + ")");
        if (checkCount == 3) {
            throw new ConditionException("Error in loop condition check");
        }
        return i < 5;
    } catch (ConditionException e) {
        print("Caught in condition: " + e.getMessage());
        return false;
    }
}

function main(): void {
    print("Testing exception in for-loop condition");

    for (Int i = 0; checkCondition(i); i = i + 1) {
        print("Loop iteration: " + i);
    }

    print("Test completed");
}

main();
