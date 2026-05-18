// Test: an uninitialized nullable field defaults to null and remains
// observable through reassignment back and forth.

class Inner {
    public int data;
    constructor(int d) {
        this.data = d;
    }
}

class Holder {
    public Inner? maybe;
}

Holder h = new Holder();
if (h.maybe == null) {
    print("default: null");
}

h.maybe = new Inner(42);
Inner? assigned = h.maybe;
if (assigned != null) {
    print("after assign: " + assigned.data);
}

h.maybe = null;
if (h.maybe == null) {
    print("after reassign null: null");
}

print("Nullable field default-to-null tests passed!");
