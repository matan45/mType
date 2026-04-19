// MYT-191: primary acceptance benchmark for the inline SET_FIELD fast path.
// Single-class hot loop that writes a field directly (not through a
// constructor) so the SET_FIELD lives inside JIT-compiled code and reaches
// tryEmitInlinedFieldSet. object_alloc.mt can't validate this path — its
// SET_FIELDs are inside Point.constructor, which is interpreted.
// Expect --jit-stats "Field IC (SET) Inline hits" to dominate on MONO.

class Box {
    public int x;
    public constructor() {
        this.x = 0;
    }
}

Box b = new Box();

int iterations = 2000000;
for (int i = 0; i < iterations; i = i + 1) {
    b.x = i;
}

print("field_write_hot x=" + b.x);
