// MYT-384: a break inside a loop nested in a switch case binds to the loop
// (post MYT-382), so the break-guard narrows the nullable for the rest of the
// loop body — the post-guard receiver use must typecheck.

class Box {
    public int value;
    constructor(int v) { this.value = v; }
    public function getValue(): int { return this.value; }
}

function maybeAt(int i, int nullAt): Box? {
    if (i == nullAt) { return null; }
    return new Box(i);
}

int sel = 0;
switch (sel) {
    case 0:
        for (int i = 0; i < 5; i = i + 1) {
            Box? b = maybeAt(i, 2);
            if (b == null) {
                break;            // exits the loop (MYT-382), so guard narrows b below
            }
            print(b.getValue());  // b narrowed to non-null — must typecheck
        }
        print("after loop in case 0");
        break;
}
print("after switch");
