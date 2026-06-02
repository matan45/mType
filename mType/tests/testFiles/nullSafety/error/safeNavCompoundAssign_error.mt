// Error: safe navigation '?.' is not supported as a compound-assignment
// target — the read half of `obj?.field += x` would be a nullable value used
// in arithmetic, which null-safety forbids (MYT-374).

class Counter {
    public int count;
    constructor(int c) { this.count = c; }
}

function main(): void {
    Counter? c = new Counter(5);
    c?.count += 10; // '?.' not allowed as a compound-assignment target
}

main();
