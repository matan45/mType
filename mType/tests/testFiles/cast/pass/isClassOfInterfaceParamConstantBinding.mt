// MYT-44: a class that implements an interface with a CONSTANT binding
// (no forwarding through a type parameter) still discriminates correctly.
// `class IntList implements MyList<Int>` has no type parameters of its
// own — the "Int" in the implements clause is a concrete class name
// that substitution leaves verbatim.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface MyList<T> {
    function get(int i): T;
}

class IntList implements MyList<Int> {
    Int stored;
    public constructor(Int v) { this.stored = v; }
    public function get(int i): Int { return this.stored; }
}

IntList il = new IntList(new Int(7));

print(il isClassOf MyList<Int>);    // true — substitution leaves "Int" verbatim
print(il isClassOf MyList<String>); // false — different concrete arg
print(il isClassOf MyList);         // true — raw fallback
