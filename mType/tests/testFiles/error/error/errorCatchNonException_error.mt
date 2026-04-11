// MYT-38: A `catch` clause whose declared type does not extend Exception
// must be rejected. Today this is enforced inside compileCatchBlocks
// (TryStatementHelper) at compile time.
//
// This is the matching pair to errorThrowPrimitive_error.mt (which covers
// throw-side rejection). Together they pin both sides of the type-system
// invariant: only Exception subtypes participate in throw/catch.
import * from "../../lib/exceptions/Exception.mt";

class NotAnException {
    public string label;
    public constructor(string l) {
        this.label = l;
    }
}

function main(): void {
    try {
        throw new Exception("ignored");
    } catch (NotAnException e) {
        // ERROR: NotAnException does not extend Exception, so this catch
        // clause must be rejected at compile time.
        print("should never run: " + e.label);
    }
}

main();
