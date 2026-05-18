// Test: a @Throw-annotated method actually throws; the caller's typed
// catch matches and finally runs after the catch. Exercises the bridge
// between the declarative @Throw annotation and the runtime try/catch/
// finally machinery on the happy "caught locally" path.
import * from "../../lib/exceptions/Exception.mt";

class ValidationException extends Exception {
    constructor(string m): super(m) {}
}

class Validator {
    @Throw(exceptions = [ValidationException])
    public function check(string s): void {
        if (s == "") {
            throw new ValidationException("empty input");
        }
    }
}

function main(): void {
    Validator v = new Validator();
    try {
        print("before");
        v.check("");
        print("unreachable");
    } catch (ValidationException e) {
        print("caught: " + e.getMessage());
    } finally {
        print("finally");
    }
    print("after");
}
main();
