// Test: @Throw annotation applied to a constructor — should fail.
// @Throw declares which exception classes a method/function may raise; a
// constructor is a distinct AnnotationHostKind and the annotation has no
// meaning there. Expected diagnostic: "cannot be applied to".
import * from "../../lib/exceptions/Exception.mt";

class MyException extends Exception {
    constructor(string message): super(message) {
    }
}

class Holder {
    @Throw(exceptions = [MyException])
    public constructor() {
    }
}

// This should never execute due to validation error
Holder h = new Holder();
print(h);
