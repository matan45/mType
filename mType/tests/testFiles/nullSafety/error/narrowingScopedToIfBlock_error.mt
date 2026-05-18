// Test: narrowing is scoped to the if-block — it must not leak afterwards.

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

Box? b = new Box(42);
if (b != null) {
    print("inside: " + b.getValue());
}
// b is back to Box? out here — method call should fail.
print("outside: " + b.getValue());
