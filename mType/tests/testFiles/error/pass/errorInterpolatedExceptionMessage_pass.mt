// Test: string interpolation reads a caught exception's message and a
// loop counter inside a catch handler.
import * from "../../lib/exceptions/Exception.mt";

function processStep(int step): void {
    if (step % 3 == 0) {
        throw new Exception("step failed");
    }
}

function main(): void {
    for (int i = 1; i <= 4; i++) {
        try {
            processStep(i);
            print($"step {i} ok");
        } catch (Exception e) {
            print($"Failure at step {i}: {e.getMessage()}");
        }
    }
}
main();
