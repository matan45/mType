// Test: the idiomatic workaround for member-access narrowing — copy the
// field into a local VariableNode, then narrow on the local.

class Inner {
    public int data;
    constructor(int d) {
        this.data = d;
    }
    public function getData(): int {
        return this.data;
    }
}

class Outer {
    public Inner? inner;
    constructor(Inner? i) {
        this.inner = i;
    }
}

Outer o = new Outer(new Inner(42));
Inner? local = o.inner;
if (local != null) {
    print("data: " + local.getData());
}

Outer empty = new Outer(null);
Inner? maybe = empty.inner;
if (maybe == null) {
    print("inner is null");
} else {
    print("should not reach here: " + maybe.getData());
}

print("Local copy of field narrowing tests passed!");
