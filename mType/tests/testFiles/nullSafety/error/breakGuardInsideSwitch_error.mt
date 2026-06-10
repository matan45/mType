// Test: MYT-381 — inside a switch, `break` binds to the switch, not the loop,
// so a break-guard must NOT narrow: execution lands after the switch (still in
// the loop body) with the variable possibly null.

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
    switch (i) {
        case 0:
            if (b == null) {
                break;  // exits the switch only — b may still be null below
            }
            break;
    }
    print(b.getValue());  // ERROR: nullable receiver
}
