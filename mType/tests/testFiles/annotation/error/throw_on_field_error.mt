// Test: @Throw annotation applied to a field — should fail.
// @Throw declares which exception classes a method/function may raise.
// Applying it to a field has no meaning and must be rejected with
// "cannot be applied to" (matches the meta-target convention used by
// parameter_target_violation_error.mt and meta_target_violation_error.mt).
import * from "../../lib/exceptions/Exception.mt";

class MyException extends Exception {
    constructor(string message): super(message) {
    }
}

class Holder {
    @Throw(exceptions = [MyException])
    public int counter = 0;
}

// This should never execute due to validation error
Holder h = new Holder();
print(h.counter);
