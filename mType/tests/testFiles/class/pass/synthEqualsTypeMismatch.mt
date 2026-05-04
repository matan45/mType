// MYT-274: synthesized equals(Object) rejects non-instance arguments via the
// `isClassOf` guard, never throwing on wrong types.

class Box {
    private int v;
    public constructor(int v) {
        this.v = v;
    }
}

class Crate {
    private int v;
    public constructor(int v) {
        this.v = v;
    }
}

Box b = new Box(42);
Crate c = new Crate(42);

print("box vs crate: " + b.equals(c));
