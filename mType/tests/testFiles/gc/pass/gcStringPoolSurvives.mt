// Test: pooled strings held by a live object survive heavy allocation
// pressure (multiple GC cycles run while the garbage graph churns).

class Holder {
    private string kept;
    constructor(string s) {
        this.kept = s;
    }
    public function getKept(): string {
        return this.kept;
    }
}

class Garbage {
    private Garbage? next;
    private string payload;
    constructor(string p) {
        this.payload = p;
        this.next = null;
    }
    public function setNext(Garbage? n): void { this.next = n; }
}

function churn(): void {
    for (int i = 0; i < 20000; i++) {
        Garbage g = new Garbage("garbage-" + i);
        Garbage h = new Garbage("garbage2-" + i);
        g.setNext(h);
        h.setNext(g);
    }
}

function main(): void {
    Holder holder = new Holder("kept-alive-" + 42);
    churn();
    print(holder.getKept());
    print(holder.getKept() == "kept-alive-42");
    print("String pool survives GC test passed!");
}
main();
