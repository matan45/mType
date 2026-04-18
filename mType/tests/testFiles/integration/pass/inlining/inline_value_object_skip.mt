// MYT-163 F-a: value-class receivers must fall through to the generic
// jit_call_method_ic path. Current IC population never creates an entry for
// ValueObject receivers, so the site stays UNINITIALIZED / MEGAMORPHIC and
// the inliner bails. Correctness check: results match the interpreter.

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
int total = 0;
for (int i = 0; i < 200; i = i + 1) {
    total = total + p.sum();
}
print(total);
