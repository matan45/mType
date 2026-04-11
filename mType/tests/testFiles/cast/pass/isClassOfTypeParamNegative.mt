// Test: the false case of `obj isClassOf T`. A Container<Int> asked
// whether an unrelated user-class instance is a T returns false — T
// is bound to Int, not to anything compatible with Animal.

import * from "../../lib/primitives/Int.mt";

class Animal {
    public string name;
    public constructor(string name) { this.name = name; }
}

class Container<T> {
    T value;
    public constructor(T v) { this.value = v; }
    public function isMyT(Object o): bool {
        return o isClassOf T;
    }
}

Container<Int> box = new Container<Int>(new Int(7));
Animal zebra = new Animal("zoe");

print(box.isMyT(zebra));              // false — Animal is not Int
print(box.isMyT(new Int(42)));        // true
print(box.isMyT(new Animal("rex")));  // false
