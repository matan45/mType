// Test: Catching abstract base exception class (valid)
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/exceptions/NullPointerException.mt";
import * from "../../lib/exceptions/IllegalArgumentException.mt";

function testCatchAbstractBase(int testCase): void {
    try {
        if (testCase == 1) {
            throw new NullPointerException("Null error");
        } else if (testCase == 2) {
            throw new IllegalArgumentException("Invalid arg");
        } else {
            throw new RuntimeException("Runtime error");
        }
    } catch (Exception e) {
        // Valid: Catching abstract base class catches all derived types
        print("Caught exception: " + e.getMessage());
    }
}

print("Testing abstract exception catching");
testCatchAbstractBase(1);
testCatchAbstractBase(2);
testCatchAbstractBase(3);
print("Test completed");
