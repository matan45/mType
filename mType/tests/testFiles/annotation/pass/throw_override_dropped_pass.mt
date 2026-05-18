// Test: an @Override method drops the parent's @Throw entirely.
// Today this is accepted — the override doesn't have to re-declare or
// honor the parent's throws contract. Pinning current behavior so any
// future LSP-style enforcement surfaces deliberately.
import * from "../../lib/exceptions/Exception.mt";

class IOException extends Exception {
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
    public function load(): string {
        return "child";
    }
}

Child c = new Child();
print(c.load());
