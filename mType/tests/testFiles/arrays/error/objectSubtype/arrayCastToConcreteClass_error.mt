// MYT-281: an array widened to Object cannot be narrowed to a concrete
// class type. The runtime cast handler's array branch rejects any target
// that is neither "Object" nor an array form, regardless of whether the
// type checker accepted the downcast at compile time.
print("Testing array cast to concrete class");

class Foo {
    public int x;
    constructor() {
        this.x = 0;
    }
}

int[] a = new int[1];
Object o = a;

// At runtime: val is array-tagged; target is "Foo" (not "Object", not
// array form) — the array branch in TypeExecutor::handleCast throws.
Foo f = (Foo)o;

print("This should not be reached");
