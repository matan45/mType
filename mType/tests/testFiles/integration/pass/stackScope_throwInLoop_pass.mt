// Exception thrown mid-loop with active STACK_SCOPE_ENTER. The throw bypasses
// the syntactic STACK_SCOPE_LEAVE, but frame teardown's releaseStackObjects
// reclaims all live slots (and resets stackObjectScopeDepth). After catch,
// continued allocations must succeed without leaking.

import * from "../../lib/exceptions/Exception.mt";

class Point {
    public int x;
    public int y;
    public constructor(int a, int b) {
        this.x = a;
        this.y = b;
    }
}

int caught = 0;
int afterCatchTotal = 0;
try {
    for (int i = 0; i < 50; i = i + 1) {
        Point p = new Point(i, i);
        if (i == 25) {
            throw new Exception("midway");
        }
    }
} catch (Exception e) {
    caught = 1;
}

// New scope must still work — the catch-handler resumed and these new
// allocations must not stomp on the leaked slots from before the throw.
for (int j = 0; j < 50; j = j + 1) {
    Point q = new Point(j, j);
    afterCatchTotal = afterCatchTotal + q.x;
}
// Sum of j in [0, 50) = 1225.
print("caught=" + caught);
print("afterCatchTotal=" + afterCatchTotal);
