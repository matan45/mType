// Test: @Throw annotation applied to a class declaration — should fail.
// @Throw is a method/function-level annotation; classes don't "throw".
// Expected diagnostic: "cannot be applied to".
import * from "../../lib/exceptions/Exception.mt";

class MyException extends Exception {
    constructor(string message): super(message) {
    }
}

@Throw(exceptions = [MyException])
class Foo {
    public constructor() {
    }
}

// This should never execute due to validation error
Foo f = new Foo();
print(f);
