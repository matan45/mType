// MYT-111 regression: exception thrown inside a constructor body invoked
// via Constructor.newInstance (reflective) must be catchable in the caller
// frame above the reflective invocation boundary.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/exceptions/Exception.mt";

class Exploder {
    public constructor(int trigger) {
        if (trigger != 0) {
            throw new Exception("ctor-boom");
        }
    }
}

Class cls = Class::forName("Exploder");
Constructor ctor = cls.getDeclaredConstructor(1);
Object[] args = new Object[1];
args[0] = 7;

try {
    Object e = ctor.newInstance(args);
    print("FAIL: no exception propagated");
} catch (Exception e) {
    print("caught: " + e.getMessage());
}
print("done");
