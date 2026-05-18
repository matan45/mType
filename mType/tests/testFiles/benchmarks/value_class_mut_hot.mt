// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Companion to inline_value_object_hot.mt (read-only value-class receiver).
// This benchmark drives the field-writing variant — bumpAndGet mutates this.n
// inside the call. Per mType value-class semantics (see
// inline_value_object_write_skip.mt) the outer instance is never mutated:
// each call always returns 1 (bumps a fresh 0 -> 1). This isolates the
// ValueObject materialise-and-discard cost from the read-only inline path.
//
// MYT-346: this benchmark currently FAILS under --jit. The --no-jit reference
// produces total=2000000 (correct, matches the existing N=200
// inline_value_object_write_skip.mt). JIT-on produces total=500 — the OSR-
// compiled loop body diverges from the interpreter on value-class write
// methods past the OSR threshold. Kept in the suite as a regression canary
// until the OSR path is fixed.

value class Counter {
    public int n;

    public constructor(int n) {
        this.n = n;
    }

    public function bumpAndGet(): int {
        this.n = this.n + 1;
        return this.n;
    }
}

Counter c = new Counter(0);

int N = 2000000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    total = total + c.bumpAndGet();
}
print("value_class_mut_hot total=" + total);
