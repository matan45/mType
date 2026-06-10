// Test: MYT-381 — a break-guard narrows only inside the loop body; after the
// loop the variable is nullable again (the break path lands right here).

class Box {
    public int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function maybe(int i): Box? {
    if (i == 0) {
        return null;
    }
    return new Box(i);
}

Box? b = maybe(0);
for (int i = 0; i < 3; i = i + 1) {
    if (b == null) {
        break;
    }
    print(b.getValue());  // OK — narrowed inside the loop body
}
// b is back to Box? out here — method call should fail.
print(b.getValue());  // ERROR: nullable receiver
