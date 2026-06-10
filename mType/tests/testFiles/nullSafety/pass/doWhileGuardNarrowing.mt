// Test: MYT-381 — continue/break guards narrow inside do-while bodies too

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

// Test 1: do-while + continue guard (increment precedes the guard so the
// loop always advances)
print("Test 1: do-while + continue guard");
int i = 0;
int sum = 0;
do {
    Box? b = maybeAt(i, 1);
    i = i + 1;
    if (b == null) {
        continue;
    }
    sum = sum + b.getValue();
} while (i < 4);
print(sum);

// Test 2: do-while + break guard
print("Test 2: do-while + break guard");
int j = 0;
do {
    Box? b = maybeAt(j, 2);
    j = j + 1;
    if (b == null) {
        break;
    }
    print(b.getValue());
} while (j < 5);
print("after do-while");

print("Do-while guard tests passed!");
