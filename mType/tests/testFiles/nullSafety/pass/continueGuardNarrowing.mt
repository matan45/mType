// Test: MYT-381 — if (x == null) { continue; } narrows x for the rest of the loop body

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

// Test 1: for + continue guard (Jira repro shape)
print("Test 1: for + continue guard");
for (int i = 0; i < 3; i = i + 1) {
    Box? b = maybeAt(i, 0);
    if (b == null) {
        continue;
    }
    print(b.getValue());
}

// Test 2: while + continue guard
print("Test 2: while + continue guard");
int n = 0;
while (n < 3) {
    Box? b = maybeAt(n, 0);
    n = n + 1;
    if (b == null) {
        continue;
    }
    print(b.getValue());
}

// Test 3: guard with a statement before the exit (block recursion)
print("Test 3: statement before continue");
for (int i = 0; i < 3; i = i + 1) {
    Box? b = maybeAt(i, 0);
    if (b == null) {
        print("skip");
        continue;
    }
    print(b.getValue());
}

// Test 4: nested composite guard — both branches exit the iteration
print("Test 4: nested exit guard");
for (int i = 0; i < 4; i = i + 1) {
    Box? b = maybeAt(i, 2);
    if (b == null) {
        if (i > 1) {
            continue;
        } else {
            break;
        }
    }
    print(b.getValue());
}

print("Continue guard tests passed!");
