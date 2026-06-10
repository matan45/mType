// Test: MYT-381 — if (x == null) { break; } narrows x for the rest of the loop body

class Box {
    public int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function maybeAt(int i, int nullAt): Box? {
    if (i == nullAt) {
        return null;
    }
    return new Box(i);
}

// Test 1: for + break guard (Jira comment repro shape)
print("Test 1: for + break guard");
for (int i = 0; i < 5; i = i + 1) {
    Box? b = maybeAt(i, 2);
    if (b == null) {
        break;
    }
    print(b.getValue());
}
print("after for");

// Test 2: while (true) + break guard
print("Test 2: while true + break guard");
int n = 0;
while (true) {
    Box? b = maybeAt(n, 3);
    n = n + 1;
    if (b == null) {
        break;
    }
    print(b.getValue());
}
print("after while");

print("Break guard tests passed!");
