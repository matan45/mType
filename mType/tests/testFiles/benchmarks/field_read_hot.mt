// MYT-194: primary acceptance benchmark for the GET_FIELD_CACHED interpreter
// fast path. Mirrors field_write_hot.mt — single-class hot loop that reads a
// field directly so GET_FIELD lives in JIT-compiled code AND (when JIT is off
// or bails) exercises the interpreter GET_FIELD_CACHED promoted opcode.
// Expect measurable improvement vs the generic GET_FIELD IC probe on MONO.

class Box {
    public int x;
    public constructor() {
        this.x = 42;
    }
}

Box b = new Box();

int iterations = 2000000;
int sum = 0;
for (int i = 0; i < iterations; i = i + 1) {
    sum = sum + b.x;
}

print("field_read_hot sum=" + sum);
