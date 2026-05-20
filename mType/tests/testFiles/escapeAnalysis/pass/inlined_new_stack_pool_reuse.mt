// MYT-352 canary: the JIT inliner now accepts callees containing NEW_STACK.
// The compiler wraps every NEW_STACK-containing block (function-body blocks
// included) with STACK_SCOPE_ENTER / STACK_SCOPE_LEAVE, and every return path
// drains open scopes before transferring control. Together these bound the
// inlined NEW_STACK's slot lifetime to one inlined call — pool slots recycle
// on every iteration instead of saturating the 32-slot per-frame cap after
// ~16 iters and falling back to the heap path.
//
// The .mt declares Point, distanceSq, and runWorkload; the C++ callback in
// EscapeAnalysisTestSuite drives runWorkload and asserts ObjectInstancePool
// hit-rate stays high after warmup. Without either landing the callback
// would observe poolMisses climbing linearly with iteration count.

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

function runWorkload(int n): int {
    int total = 0;
    for (int i = 0; i < n; i = i + 1) {
        total = total + distanceSq(i, i + 1, i + 2, i + 3);
    }
    return total;
}
