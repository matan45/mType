// MYT-208 escape-analysis Phase 2 target: short-scope allocations inside a
// helper function called in a hot loop. Each call's frame tears down
// immediately — Point's lifetime is bounded by distanceSq, so the analyzer
// promotes both `new Point(...)` allocations to NEW_STACK and pool slots
// recycle on every iteration.
//
// Acceptance: min wall-clock at least 20% lower than the same workload run
// against the MYT-134-only baseline (which still wraps NEW_STACK in a
// shared_ptr and emits OBJECT-tagged Values).

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

int N = 2000000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    total = total + distanceSq(i, i + 1, i + 2, i + 3);
}
print("object_alloc_nested total=" + total);
