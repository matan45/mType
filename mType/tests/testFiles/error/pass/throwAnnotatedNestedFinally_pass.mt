// Test: a @Throw-annotated method throws from inside an inner try with
// only a finally (no catch); the outer try's catch matches and outer
// finally runs last. Exercises finally ordering when @Throw bridges
// nested handler frames.
import * from "../../lib/exceptions/Exception.mt";

class DataError extends Exception {
    constructor(string m): super(m) {}
}

class Loader {
    @Throw(exceptions = [DataError])
    public function load(): void {
        throw new DataError("boom");
    }
}

function main(): void {
    Loader l = new Loader();
    try {
        try {
            print("inner try");
            l.load();
            print("inner unreachable");
        } finally {
            print("inner finally");
        }
    } catch (DataError e) {
        print("outer caught: " + e.getMessage());
    } finally {
        print("outer finally");
    }
}
main();
