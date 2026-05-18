// Test: a @Throw-annotated method declares two exception types and
// throws the second; the second of two catch clauses (typed on the
// specific subtype) matches, and finally runs exactly once.
import * from "../../lib/exceptions/Exception.mt";

class FormatError extends Exception {
    constructor(string m): super(m) {}
}

class RangeError extends Exception {
    constructor(string m): super(m) {}
}

class Parser {
    @Throw(exceptions = [FormatError, RangeError])
    public function parseRange(int x): int {
        if (x < 0) {
            throw new RangeError("negative");
        }
        if (x > 100) {
            throw new FormatError("too large");
        }
        return x;
    }
}

function main(): void {
    Parser p = new Parser();
    try {
        p.parseRange(-1);
    } catch (FormatError e) {
        print("format: " + e.getMessage());
    } catch (RangeError e) {
        print("range: " + e.getMessage());
    } finally {
        print("finally");
    }
}
main();
