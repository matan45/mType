// TARGET: GC cycle-collection workload.
// Repeatedly creates short-lived cyclic object graphs in a helper scope so
// normal VM safepoints can collect unreachable cycles during the benchmark.

class GCNode {
    public int value;
    public GCNode next;
    public GCNode other;

    public constructor(int value) {
        this.value = value;
        this.next = null;
        this.other = null;
    }

    public function link(GCNode next, GCNode other): void {
        this.next = next;
        this.other = other;
    }
}

function makeCycle(int seed): int {
    int base = seed % 97;

    GCNode a = new GCNode(base);
    GCNode b = new GCNode(base + 1);
    GCNode c = new GCNode(base + 2);
    GCNode d = new GCNode(base + 3);

    a.link(b, c);
    b.link(c, d);
    c.link(d, a);
    d.link(a, b);

    return a.value + a.next.value + b.next.value + c.next.value;
}

int N = 40000;
int checksum = 0;
for (int i = 0; i < N; i = i + 1) {
    checksum = checksum + makeCycle(i);
}

print("gc_cycle_churn checksum=" + checksum);
