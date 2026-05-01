// Test: a `public final function` that throws is caught normally.
// Final on the method affects override semantics, not throw propagation.
import * from "../../lib/exceptions/Exception.mt";

class Service {
    public final function run(): void {
        throw new Exception("svc");
    }
}

function main(): void {
    Service s = new Service();
    try {
        s.run();
    } catch (Exception e) {
        print("caught " + e.getMessage());
    }
    print("done");
}
main();
