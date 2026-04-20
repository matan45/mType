// MYT-198: fusion must NOT fire when the CACHED opcode is itself a jump target.
// The `if` branch here generates a conditional jump that lands on the method
// call offset in some branch arrangements; isFusionUnsafeTarget should veto
// the fusion so behavior is preserved. Output must match un-fused dispatch.

class Tally {
    public int n;
    public constructor() { this.n = 0; }
    public function add(int x): int {
        this.n = this.n + x;
        return this.n;
    }
}

Tally t = new Tally();
int acc = 0;
for (int i = 0; i < 1000; i = i + 1) {
    if (i % 2 == 0) {
        acc = acc + t.add(i);
    } else {
        acc = acc + t.add(-i);
    }
}
print(acc);
print(t.n);
