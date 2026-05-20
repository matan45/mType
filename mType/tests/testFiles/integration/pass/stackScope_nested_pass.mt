// Per-scope NEW_STACK release across two nested blocks. Each iteration of
// the outer loop opens its own stack-scope; the inner loop opens another.
// Both must release at their respective scope-exits so neither overflows
// CallFrame::kStackObjectsCap nor stomps the other scope's slots.

class Point {
    public int x;
    public int y;
    public constructor(int a, int b) {
        this.x = a;
        this.y = b;
    }
}

int outerN = 40;
int innerN = 40;
int total = 0;
for (int i = 0; i < outerN; i = i + 1) {
    Point outer = new Point(i, i);
    int inner_total = 0;
    for (int j = 0; j < innerN; j = j + 1) {
        Point inner = new Point(j, j + 1);
        inner_total = inner_total + inner.x + inner.y;
    }
    total = total + outer.x + outer.y + inner_total;
}
// Inner sum per outer iteration: sum over j in [0, innerN) of j + (j+1)
//   = innerN*(innerN-1) + innerN = innerN^2 = 1600.
// Outer contribution per iter: 2*i. Sum over i in [0, outerN): outerN*(outerN-1) = 1560.
// Total = 1560 + outerN * 1600 = 1560 + 64000 = 65560.
print("total=" + total);
