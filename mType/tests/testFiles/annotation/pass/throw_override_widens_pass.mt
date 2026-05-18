// Test: @Override declares a superset of exceptions vs the parent's @Throw.
// Today the validator does NOT compare exception sets between parent and
// child; only signature matching is enforced. This pass test pins that
// permissive behavior so a future tightening (e.g., LSP-style covariance
// of throws) surfaces as a deliberate change.
import * from "../../lib/exceptions/Exception.mt";

class IOException extends Exception {
    constructor(string m): super(m) {}
}

class NetworkException extends Exception {
    constructor(string m): super(m) {}
}

class Parent {
    @Throw(exceptions = [IOException])
    public function load(): string {
        return "parent";
    }
}

class Child extends Parent {
    @Override
    @Throw(exceptions = [IOException, NetworkException])
    public function load(): string {
        return "child";
    }
}

Child c = new Child();
print(c.load());
