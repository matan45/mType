// Test: Lambda capturing exception object from outer scope
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";

interface Action {
    function execute(): void;
}

function main(): void {
    print("Testing lambda capturing exception object");

    Exception capturedException = null;

    try {
        throw new Exception("Captured exception message");
    } catch (Exception e) {
        capturedException = e;
        print("Exception caught in outer scope: " + e.getMessage());
    }

    // Lambda captures the exception from outer scope
    Action action = () -> {
        if (capturedException != null) {
            print("Lambda accessing captured exception: " + capturedException.getMessage());
            print("Exception message length: " + capturedException.getMessage().length());
        } else {
            print("No exception was captured");
        }
    };

    action.execute();

    // Modify captured exception reference
    capturedException = new Exception("New exception");

    // Lambda sees the updated reference
    action.execute();

    print("Lambda exception capturing test completed");
}

main();
