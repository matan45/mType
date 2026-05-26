// MYT-369: returning a boxed primitive (Int wrapper) from a function declared
// to return the matching primitive (int) must auto-unbox at the return site.
// Pre-fix, validateReturnType accepted the source but no UNBOX bytecode was
// emitted, so the caller received an OBJECT in an int-tagged slot.
import * from "../../lib/primitives/Int.mt";

class Holder {
    public Int boxed;

    public constructor(int v) {
        this.boxed = new Int(v);
    }
}

function unwrap(Holder h): int {
    return h.boxed;
}

Holder h = new Holder(7);
int result = unwrap(h);
print(result);
print(result + 1);
