// Test: an unused method decorated with @Throw on a class that is itself
// used must survive DCE and remain reflectable. Asserts both that the
// method exists post-optimization (Class::forName + getDeclaredMethod
// succeeds) and that the @Throw annotation is still attached.
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

class MyError extends Exception {
    constructor(string m): super(m) {}
}

class Service {
    public function used(): string {
        return "used";
    }

    @Throw(exceptions = [MyError])
    public function unused(): void {
        // No direct call site; @Throw guards it from DCE.
    }
}

function main(): void {
    Service s = new Service();
    print(s.used());
    Class c = Class::forName("Service");
    Method m = c.getDeclaredMethod("unused", 0);
    print(m.hasAnnotation("Throw"));
}
main();
