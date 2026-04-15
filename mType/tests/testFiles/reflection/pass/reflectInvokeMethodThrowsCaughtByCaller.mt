// MYT-111 regression: exception thrown inside a reflectively-invoked
// instance method body must be catchable in the caller frame above the
// reflective invocation boundary. Previously the exception was silently
// swallowed because VirtualMachine::invokeMethod restored the saved
// callStack after the handler had unwound it.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/exceptions/Exception.mt";

class Boomer {
    public constructor() { }

    public function boom(): void {
        throw new Exception("kaboom");
    }
}

Class cls = Class::forName("Boomer");
Method m = cls.getDeclaredMethod("boom", 0);
Constructor ctor = cls.getDeclaredConstructor(0);
Object[] noArgs = new Object[0];
Object inst = ctor.newInstance(noArgs);

try {
    m.invoke(inst, noArgs);
    print("FAIL: no exception propagated");
} catch (Exception e) {
    print("caught: " + e.getMessage());
}
print("done");
