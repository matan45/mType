// Test: compound `&&` conditions do NOT propagate narrowing.
// The tracker only inspects a top-level EQUALS / NOT_EQUALS against
// NullNode; once the condition is `&&`, the analyzer short-circuits and
// `b` stays nullable in the then-branch.

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
print("Should not reach here");
