// Test exception handling in for-loop increment
import * from "../../lib/exceptions/Exception.mt";

class IncrementException extends Exception {
    constructor(string message): super(message) {
    }
}

function computeIncrement(int current): int {
    try {
        print("Computing increment for i=" + current);
        if (current == 2) {
            throw new IncrementException("Error computing increment at i=2");
        }
        return current + 1;
    } catch (IncrementException e) {
        print("Caught in increment: " + e.getMessage());
        return current + 2;
    }
}

function main(): void {
    print("Testing exception in for-loop increment");

    for (int i = 0; i < 5; i = computeIncrement(i)) {
        print("Loop iteration: " + i);
    }

    print("Test completed");
}

main();
