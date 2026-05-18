// Test: two @Throw annotations stacked on the same method — should fail.
// A method has at most one throws contract; duplicate @Throw is ambiguous
// (which list wins?) and must be rejected rather than silently merged
// or shadowed.
import * from "../../lib/exceptions/Exception.mt";

class A extends Exception {
    constructor(string m): super(m) {}
}

class B extends Exception {
    constructor(string m): super(m) {}
}

class Holder {
    @Throw(exceptions = [A])
    @Throw(exceptions = [B])
    public function doWork(): void {
    }
}

// This should never execute due to validation error
Holder h = new Holder();
h.doWork();
