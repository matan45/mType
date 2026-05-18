// TARGET: ~2-4s on dev machine. Adjust N if first run lands outside 1-5s.
// Companion to gc_cycle_churn.mt — that one creates cyclic graphs; this one
// drives raw allocation pressure through short-lived non-cyclic objects.
// Three allocations per iter, all dying at function-return — the escape
// analysis / field-init-template path should dominate.

class Box {
    public int v;

    public constructor(int v) {
        this.v = v;
    }
}

function alloc(int x): int {
    Box a = new Box(x);
    Box b = new Box(x + 1);
    Box c = new Box(x + 2);
    return a.v + b.v + c.v;
}

int N = 1000000;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    acc = acc + alloc(i);
}
print("gc_churn_intense_hot acc=" + acc);
