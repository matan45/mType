// Test: a @Throw-annotated method throws; the exception propagates
// through an UNannotated intermediate caller (which has its own finally)
// and is caught two frames up. Verifies inner-finally-before-outer-catch
// ordering for the @Throw integration path.
import * from "../../lib/exceptions/Exception.mt";

class ConfigError extends Exception {
    constructor(string m): super(m) {}
}

class Source {
    @Throw(exceptions = [ConfigError])
    public function load(): string {
        throw new ConfigError("missing key");
    }
}

class Fetcher {
    // Intentionally NOT @Throw-annotated — pinning that absence of the
    // annotation on an intermediate caller does not block propagation.
    public function fetch(Source s): string {
        try {
            return s.load();
        } finally {
            print("inner finally");
        }
    }
}

function main(): void {
    Source s = new Source();
    Fetcher f = new Fetcher();
    try {
        string v = f.fetch(s);
        print("unreachable: " + v);
    } catch (ConfigError e) {
        print("outer caught: " + e.getMessage());
    } finally {
        print("outer finally");
    }
}
main();
