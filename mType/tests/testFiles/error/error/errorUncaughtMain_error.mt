// MYT-38: A throw with no surrounding catch must propagate out of `main`
// and surface as a runtime error. Every existing exception test wraps
// throws in try/catch — none exercises the "let it escape" path that the
// VM's default uncaught-exception handling drives.
//
// Test framework: registered with the new ERROR_EXPECTED + substring
// overload (MYT-38). UserException::what() formats as
// `"User exception thrown: <typeName>"` (UserException.hpp:33), so the
// substring used to assert this test is the unique sentinel subclass
// name "MytUncaughtSentinel38" — proving the runtime error came from
// THIS throw and not from some unrelated parse/setup failure.
import * from "../../lib/exceptions/Exception.mt";

class MytUncaughtSentinel38 extends Exception {
    public constructor(string msg): super(msg) {
    }
}

function main(): void {
    throw new MytUncaughtSentinel38("uncaughtMainSentinel");
}

main();
