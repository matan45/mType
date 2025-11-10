// Error: Attempting to throw null value
import * from "../../lib/exceptions/Exception.mt";

function testThrowNull(): void {
    Exception? nullException = null;

    try {
        // ERROR: Cannot throw null
        throw nullException;
    } catch (Exception e) {
        print("Caught exception");
    }
}

testThrowNull();
