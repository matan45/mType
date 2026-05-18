// Test: a duplicate exception class inside @Throw's `exceptions` list is
// silently ignored today (AnnotationValidator::checkForDuplicates skips
// duplicates without erroring). This pass test pins the current behavior;
// if duplicate handling is tightened to an error in the future, this test
// will fail intentionally and force a deliberate update.
import * from "../../lib/exceptions/Exception.mt";

class MyException extends Exception {
    constructor(string m): super(m) {}
}

class Foo {
    @Throw(exceptions = [MyException, MyException])
    public function doWork(): void {
        print("ran");
    }
}

Foo f = new Foo();
f.doWork();
