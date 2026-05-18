// Test: a class with a nullable field can be assigned null or non-null,
// and read back via a getter that copies into a local for narrowing.

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

    public function describe(): string {
        Inner? local = this.inner;
        if (local != null) {
            return "value: " + local.data;
        }
        return "null";
    }
}

Outer o = new Outer(new Inner(5));
print(o.describe());

o.inner = null;
print(o.describe());

o.inner = new Inner(99);
print(o.describe());

Outer fromNull = new Outer(null);
print(fromNull.describe());

print("Nullable field assign-and-get tests passed!");
