// Test: `if (this.field != null) { this.field.method() }` does NOT narrow.
// The tracker only handles VariableNode names; `this.field` is a
// MemberAccessNode, so the access on the nullable receiver must fail.

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

    public function broken(): int {
        if (this.inner != null) {
            return this.inner.data;
        }
        return 0;
    }
}

Outer o = new Outer(new Inner(5));
print(o.broken());
