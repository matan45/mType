// MYT-373: || guard-clause narrowing on a hot path.
// Path under test: the OR branch of analyzeNullCondition + guard-clause
// narrowing in ControlFlowCompiler. `if (a == null || b == null) return -1;`
// produces whenFalseNonNull facts for BOTH a and b, so the fall-through
// dispatch `a.getValue() + b.getValue()` compiles on the nullable receivers.
// Distinct branch from null_narrowing_hot (which exercises the && / whenTrue
// path). TARGET: ~2s on dev machine (Release). Adjust N if outside 1-5s.

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
    // Early-exit guard: when it falls through, both a and b are narrowed.
    if (a == null || b == null) {
        return -1;
    }
    return a.getValue() + b.getValue();
}

int N = 3000000;
Box? present = new Box(3);
Box? absent = null;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    if (i % 2 == 0) {
        acc = acc + sumAfterGuard(present, present);
    } else {
        acc = acc + sumAfterGuard(present, absent);
    }
}

print("null_guard_clause_hot acc=" + acc);
