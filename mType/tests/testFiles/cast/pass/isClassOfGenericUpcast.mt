// Test: inheritance-based upcast for parameterized classes. A Dog<Int>
// is still an Animal (raw fallback) — but `dog isClassOf Animal<Int>`
// returns false in v1 because propagating type arguments through `extends`
// is not yet implemented (documented known limitation in MYT-41).

class Animal<T> {
    T tag;
    constructor(T tag) { this.tag = tag; }
}

class Dog<T> extends Animal<T> {
    constructor(T tag) { super(tag); }
}

import * from "../../lib/primitives/Int.mt";

Dog<Int> dog = new Dog<Int>(new Int(1));

print(dog isClassOf Dog);         // true
print(dog isClassOf Animal);      // true (raw base-match upcast)
print(dog isClassOf Dog<Int>);    // true
print(dog isClassOf Dog<Animal>); // false (invariant)
