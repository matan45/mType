// Test: `T = Int` via the lib/primitives Int wrapper — the case most
// likely to surface any boxing / variant-access interaction with the
// Pure OOP migration. A Container<Int>'s isMyT must accept both an Int
// instance AND a Container<Int>'s own stored value.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Float.mt";

class Container<T> {
    T value;
    public constructor(T v) { this.value = v; }

    public function isMyT(Object o): bool {
        return o isClassOf T;
    }

    public function valueIsT(): bool {
        return this.value isClassOf T;
    }
}

Container<Int> box = new Container<Int>(new Int(100));

print(box.isMyT(new Int(1)));     // true
print(box.isMyT(new Float(1.5))); // false
print(box.valueIsT());             // true
