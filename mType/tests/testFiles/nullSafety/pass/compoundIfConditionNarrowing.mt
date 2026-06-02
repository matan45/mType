// MYT-373: compound `&&` condition facts narrow into the then branch.

class Box {
    public int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function process(Box? b, int x): int {
    if (b != null && x > 0) {
        return b.getValue();
    }
    return 0;
}

print(process(new Box(5), 1));
print(process(new Box(5), 0));
print(process(null, 1));

// Expected output:
// 5
// 0
// 0
