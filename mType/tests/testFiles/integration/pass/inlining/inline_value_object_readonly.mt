// MYT-167 F-e: value-class receivers with read-only methods are now inlined.
// The IC populates receiverIsValueObject=true on the first miss, eligibility
// accepts read-only callees, and the shape-guard / inline body emits the same
// code path as ObjectInstance MONO inlining. Correctness guard: output must
// match the interpreter (7 * 200 = 1400).

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
