// Test: Guard clause WITHOUT exit should NOT narrow

class Box {
    public int value;

    constructor(int v) {
        this.value = v;
    }
}

function broken(Box? b): int {
    if (b == null) {
        print("oops");
    }
    // b is still nullable here — the if-body does not exit
    return b.value;
}

print(broken(new Box(5)));
