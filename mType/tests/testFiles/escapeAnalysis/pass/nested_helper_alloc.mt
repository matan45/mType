// MYT-208: short-scope allocations inside a helper function called in a
// loop. Both `a` and `b` are bounded by distanceSq's frame and must be
// promoted to NEW_STACK. Output is the sum of d² across N iterations of
// (i, i+1) → (i+2, i+3), which is constant per iteration: dx=-2, dy=-2,
// so each call returns 8 — total = 8 * 5 = 40.

class Point {
    public int x;
    public int y;
    public constructor(int a, int b) {
        this.x = a;
        this.y = b;
    }
}

function distanceSq(int x1, int y1, int x2, int y2): int {
    Point a = new Point(x1, y1);
    Point b = new Point(x2, y2);
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    return dx * dx + dy * dy;
}

int total = 0;
for (int i = 0; i < 5; i = i + 1) {
    total = total + distanceSq(i, i + 1, i + 2, i + 3);
}
print(total);
