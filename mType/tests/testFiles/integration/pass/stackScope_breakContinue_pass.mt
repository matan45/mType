// Per-scope NEW_STACK release with early-exit control flow. Both break and
// continue cross an active STACK_SCOPE_ENTER without a syntactic LEAVE; the
// compiler synthesizes the LEAVE before the JUMP so stackObjectsCount stays
// balanced and the pool isn't starved by leaking slots across iterations.

class Point {
    public int x;
    public int y;
    public constructor(int a, int b) {
        this.x = a;
        this.y = b;
    }
}

int N = 100;
int continued = 0;
int totalUntilBreak = 0;
for (int i = 0; i < N; i = i + 1) {
    Point p = new Point(i, i);
    if (i == 50) {
        // continue: synthesized LEAVE releases p before jumping to increment.
        continued = continued + 1;
        continue;
    }
    if (i == 70) {
        // break: synthesized LEAVE releases p before exiting the loop.
        totalUntilBreak = totalUntilBreak + p.x;
        break;
    }
    totalUntilBreak = totalUntilBreak + p.x;
}
// Hit 0..49 (sum 1225), skipped 50, 51..69 (sum 1140), break at 70 (add 70 then exit).
// Total = 1225 + 1140 + 70 = 2435.
print("totalUntilBreak=" + totalUntilBreak);
print("continued=" + continued);
