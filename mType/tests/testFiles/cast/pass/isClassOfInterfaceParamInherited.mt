// MYT-44: interface-on-interface inheritance with DIVERGENT parameter
// letters at each level. This is the rebind-on-recursion test — if you
// accidentally pass parent bindings down through the recursion instead
// of rebinding against the next interface's declared parameters, this
// test still passes when letters happen to coincide. Using `A` at the
// class level, `B` at the middle interface, and `E` at the root
// interface forces every level to rebind.

import * from "../../lib/primitives/Int.mt";

interface MyList<E> {
    function get(int i): E;
}

interface SortedList<B> extends MyList<B> {
    function sort(): void;
}

class SortedArrayList<A> implements SortedList<A> {
    A slot;
    public constructor(A v) { this.slot = v; }
    public function get(int i): A { return this.slot; }
    public function sort(): void {}
}

SortedArrayList<Int> sal = new SortedArrayList<Int>(new Int(1));

print(sal isClassOf SortedList<Int>); // true — direct via substitute
print(sal isClassOf MyList<Int>);     // true — via rebind-on-recursion
print(sal isClassOf MyList);          // true — raw fallback still works
print(sal isClassOf SortedList);      // true — raw fallback still works
