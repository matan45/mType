// Test: an unused top-level function decorated with @Throw must compile
// and the program must run normally. The DCE pass
// (UnusedDeclarationEliminationPass) is documented to preserve
// @Throw-annotated declarations alongside @Script ones; if that contract
// is broken, a future change that removes neverCalled() would still pass
// here (it's truly unused), but pairing this with the method test below
// gives coverage of the function-level path through the optimizer.
import * from "../../lib/exceptions/Exception.mt";

class MyError extends Exception {
    constructor(string m): super(m) {}
}

@Throw(exceptions = [MyError])
function neverCalled(): void {
    // No call site anywhere; only @Throw keeps it alive across DCE.
}

function main(): void {
    print("ok");
}
main();
