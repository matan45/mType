// Test empty try block - should produce error if not allowed
import * from "../../lib/exceptions/Exception.mt";

function testEmptyTry(): void {
    try {
        // Empty try block - this should be an error
    } catch (Exception e) {
        print("Caught exception");
    }
}

testEmptyTry();
