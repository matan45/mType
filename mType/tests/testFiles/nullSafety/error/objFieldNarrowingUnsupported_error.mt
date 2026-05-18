// Test: external-object field narrowing is also unsupported. The check
// `o.inner != null` does not narrow `o.inner` for subsequent field access.

class Inner {
    public int data;
    constructor(int d) {
        this.data = d;
    }
}

class Outer {
    public Inner? inner;
    constructor(Inner? i) {
        this.inner = i;
    }
}

Outer o = new Outer(new Inner(5));
if (o.inner != null) {
    print(o.inner.data);
}
print("Should not reach here");
