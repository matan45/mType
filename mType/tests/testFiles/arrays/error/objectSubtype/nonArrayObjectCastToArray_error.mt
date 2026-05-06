// MYT-281: a class instance held in an Object slot cannot be narrowed
// to an array type. The runtime cast handler's array branch is gated on
// the receiver's tag (ARRAY); a class-instance receiver falls through
// to the existing castToObject path, which rejects array-shaped targets.
print("Testing non-array Object cast to array");

class Foo {
    public int x;
    constructor() {
        this.x = 42;
    }
}

Foo f = new Foo();
Object o = f;

// At runtime: val.tag == OBJECT, not ARRAY. Falls past the array branch.
// The fallback castToObject(val, "int[]") rejects the cast.
int[] a = (int[])o;

print("This should not be reached");
