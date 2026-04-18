// MYT-167 F-e: primary acceptance benchmark for value-class inlining.
// Same shape as inline_monomorphic.mt but the receiver is a value class, so
// the slow path today is jit_call_method's temp-ObjectInstance materialisation.
// Post-F-e: the MONO IC site populates with receiverIsValueObject=true, the
// shape-guard + inline body emits, and the call frame disappears.

value class Point {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function sum(): int {
        return this.x + this.y;
    }
}

Point p = new Point(3, 4);

int iterations = 2000000;
int acc = 0;
for (int i = 0; i < iterations; i = i + 1) {
    acc = acc + p.sum();
}

print("inline_value_object_hot acc=" + acc);
