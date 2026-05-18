// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Companion to inline_value_object_hot.mt (read-only value-class receiver).
// This benchmark drives the field-writing variant — bumpAndGet mutates this.n
// inside the call. Per mType value-class semantics (see
// inline_value_object_write_skip.mt) the outer instance is never mutated:
// each call always returns 1 (bumps a fresh 0 -> 1). This isolates the
// ValueObject materialise-and-discard cost from the read-only inline path.
//
// MYT-346: pre-fix this produced total=500 under --jit (the OSR threshold);
// the inliner inlined the body against a VALUE_OBJECT-tagged local-0 and
// SET_FIELD_CACHED's CoW path wrote to the wrong slot. Fixed by teaching the
// inliner to materialise a temp ObjectInstance into local-0 for value-class
// write methods (INLINE_VALUE_REQUIRES_MATERIALISATION eligibility variant).
// Kept as both a perf benchmark and a regression canary.

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
