// Test: Chained member access through a nullable field should fail at compile time

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
Inner? maybeInner = o.inner;
print(maybeInner.data);
print("Should not reach here");
