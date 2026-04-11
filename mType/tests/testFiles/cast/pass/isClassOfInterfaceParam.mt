// MYT-44 headline test: a parameterized interface target discriminates
// between different type arguments on the same base interface.
// `nums isClassOf List<Int>` must return true for an ArrayList<Int>
// and false for an ArrayList<String>.

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

MyArrayList<Int> nums = new MyArrayList<Int>(new Int(42));
MyArrayList<String> strs = new MyArrayList<String>(new String("hi"));

print(nums isClassOf MyList<Int>);     // true
print(nums isClassOf MyList<String>);  // false
print(strs isClassOf MyList<Int>);     // false
print(strs isClassOf MyList<String>);  // true
