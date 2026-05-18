// MYT-346: regression — value-class method that writes its own field, called
// from a loop hot enough to OSR. Pre-fix the JIT inlined the body against a
// VALUE_OBJECT-tagged local-0; SET_FIELD_CACHED's CoW path wrote to the
// operand-stack slot of LOAD_LOCAL instead of local-0, so subsequent reads
// returned the unchanged 0 and total stayed at the OSR-threshold count.
// Post-fix the inliner materialises a temp ObjectInstance into local-0
// (jit_materialise_value_receiver_into_local) before emitting the body, so
// the mutation propagates correctly and every call returns 1. Companion to
// inline_value_object_write_skip.mt (N=200, sub-OSR-threshold).

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
int total = 0;
for (int i = 0; i < 1000; i = i + 1) {
    total = total + c.bumpAndGet();
}
print(total);
