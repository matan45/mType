// Error: Catch block for derived type after base type is unreachable (dead code)
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/exceptions/NullPointerException.mt";

function testDeadCatchBlock(): void {
    try {
        throw new NullPointerException("Test exception");
    } catch (Exception e) {
        // Catches all exceptions including RuntimeException and NullPointerException
        print("Caught base Exception");
    } catch (RuntimeException re) {
        // ERROR: This catch block is unreachable - Exception already caught above
        print("Caught RuntimeException");
    } catch (NullPointerException npe) {
        // ERROR: This catch block is also unreachable
        print("Caught NullPointerException");
    }
}

testDeadCatchBlock();
