// MYT-373: a compound null guard that exits narrows all guarded variables
// for subsequent statements.

class Box {
    public int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function sumAfterGuard(Box? a, Box? b): int {
    if (a == null || b == null) {
        return -1;
    }
    return a.getValue() + b.getValue();
}

print(sumAfterGuard(new Box(2), new Box(3)));
print(sumAfterGuard(null, new Box(3)));
print(sumAfterGuard(new Box(2), null));

// Expected output:
// 5
// -1
// -1
