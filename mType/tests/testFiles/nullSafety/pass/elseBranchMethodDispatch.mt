// Test: else-branch narrowing exercised through method dispatch
// `if (x == null) ... else { x.method() }` — narrowing in the else-branch
// must be strong enough to allow a method call, not just a print.

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

Box? maybeBox = new Box(42);
if (maybeBox == null) {
    print("should not happen");
} else {
    print("non-null: " + maybeBox.getValue());
}

Box? nullBox = null;
if (nullBox == null) {
    print("null branch reached");
} else {
    print("should not happen: " + nullBox.getValue());
}

print("Else-branch narrowing tests passed!");
