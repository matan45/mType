// Test: nested `if (a != null) { if (b != null) { ... } }` — both
// narrowings stack via the tracker's scope stack.

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

function combine(Box? a, Box? b): int {
    if (a != null) {
        if (b != null) {
            return a.getValue() + b.getValue();
        }
        return a.getValue();
    }
    return -1;
}

print(combine(new Box(10), new Box(20)));
print(combine(new Box(5), null));
print(combine(null, new Box(7)));
print(combine(null, null));

print("Nested narrowing tests passed!");
