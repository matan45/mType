// Test: @Throw annotation applied to a method parameter — should fail.
// Parameter annotations are recognized (HostKind::PARAMETER) but @Throw
// describes a function-level throws contract; placing it on a parameter
// is meaningless and must be rejected with "cannot be applied to".
import * from "../../lib/exceptions/Exception.mt";

class MyException extends Exception {
    constructor(string message): super(message) {
    }
}

class Foo {
    public function doWork(@Throw(exceptions = [MyException]) int x): int {
        return x;
    }
}

// This should never execute due to validation error
Foo f = new Foo();
print(f.doWork(5));
