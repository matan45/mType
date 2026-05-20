// Per-scope NEW_STACK release: ensures stack-promoted Point allocations
// inside a top-level for-loop body see correct field state every iteration.
// Highest-risk silent-corruption case if resetForRecycle leaks fields between
// pool slot reuses. Iteration count comfortably above CallFrame::kStackObjectsCap
// (32) so the per-iteration release must fire to keep slot reuse working.

class Point {
    public int x;
    public int y;
    public constructor(int a, int b) {
        this.x = a;
        this.y = b;
    }
}

int N = 200;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    Point p = new Point(i, i + 1);
    total = total + p.x + p.y;
}
// Closed form: sum of i + (i+1) for i in [0, N) = N*(N-1) + N = N^2.
print("total=" + total);
