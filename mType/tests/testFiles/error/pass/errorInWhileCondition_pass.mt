// Test exception handling in while condition
import * from "../../lib/exceptions/Exception.mt";

class WhileException extends Exception {
    constructor(string message): super(message) {
    }
}

int iterCount = 0;

function checkWhileCondition(): bool {
    iterCount = iterCount + 1;
    try {
        print("Checking while condition (iteration " + iterCount + ")");
        if (iterCount == 4) {
            throw new WhileException("Error in while condition at iteration 4");
        }
        return iterCount < 6;
    } catch (WhileException e) {
        print("Caught in while condition: " + e.getMessage());
        return false;
    }
}

function main(): void {
    print("Testing exception in while condition");

    while (checkWhileCondition()) {
        print("While loop body executing (iteration " + iterCount + ")");
    }

    print("While loop ended at iteration: " + iterCount);
    print("Test completed");
}

main();
