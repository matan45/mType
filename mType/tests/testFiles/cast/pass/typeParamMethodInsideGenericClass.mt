// MYT-228: a method-level <U> on a generic class with class-level <T> must
// resolve U through the per-frame typeArgBindings AND T through the
// receiver's class-level bindings. The rbegin→rend walk hits the per-frame
// map for U first, then falls through to the receiver bindings for T.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Box<T> {
    T value;
    public constructor(T v) { this.value = v; }

    // Class-level T resolves via receiver bindings.
    public function holdsT(Object o): bool {
        return o isClassOf T;
    }

    // Method-level U resolves via per-frame typeArgBindings.
    public function <U> probeU(Object o): bool {
        return o isClassOf U;
    }
}

Box<Int> b = new Box<Int>(new Int(42));
Int n = new Int(7);
String s = new String("hi");

// Class-level T = Int
print(b.holdsT(n));            // true
print(b.holdsT(s));            // false

// Method-level U set per call
print(b.probeU<Int>(n));       // true
print(b.probeU<Int>(s));       // false
print(b.probeU<String>(n));    // false
print(b.probeU<String>(s));    // true
