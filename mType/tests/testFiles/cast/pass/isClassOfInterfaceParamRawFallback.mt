// MYT-44 regression gate: raw-target interface check (`list isClassOf List`)
// still works after the MYT-41 parameterized-target gate comes down.
// This is the guard that catches "oops I accidentally tightened the raw
// path when wiring the parameterized branch."

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface MyList<T> {
    function get(int i): T;
}

class MyArrayList<E> implements MyList<E> {
    E slot;
    public constructor(E v) { this.slot = v; }
    public function get(int i): E { return this.slot; }
}

MyArrayList<Int> nums = new MyArrayList<Int>(new Int(1));
MyArrayList<String> strs = new MyArrayList<String>(new String("hi"));

// Raw target — both parameterized instances satisfy the base interface.
print(nums isClassOf MyList); // true
print(strs isClassOf MyList); // true
