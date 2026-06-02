// MYT-373: null-narrowing dispatch on a short-circuit && hot path.
// Path under test: BinaryOp `&&` whose left operand is a null guard
// (b != null) narrows the right operand via ScopedNullNarrowing so
// `b.getValue()` compiles + dispatches on the nullable receiver. The then
// branch of the guarded `if` is also narrowed (ControlFlowCompiler). This
// exercises the MYT-373 narrowing facts together with JIT short-circuit jump
// emission on the same site.
// TARGET: ~2s on dev machine (Release). Adjust N if first run lands outside 1-5s.

class Box {
    public int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function score(Box? b): int {
    // Left guard narrows `b` for the right operand dispatch.
    if (b != null && b.getValue() > 0) {
        // then branch: `b` narrowed to non-null here too.
        return b.getValue();
    }
    return 0;
}

int N = 4000000;
Box? present = new Box(7);
Box? absent = null;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    if (i % 2 == 0) {
        acc = acc + score(present);
    } else {
        acc = acc + score(absent);
    }
}

print("null_narrowing_hot acc=" + acc);
