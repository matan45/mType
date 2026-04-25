// MYT-208: monomorphic method call on a stack-promoted local. Exercises
// the IC fast path with a STACK_OBJECT receiver — shape lookup keys off
// ClassDefinition* (identical for OBJECT and STACK_OBJECT), so the same IC
// entry serves both.

class Counter {
    public int n;
    public constructor(int start) {
        this.n = start;
    }
    public function bumpTwice(): int {
        this.n = this.n + 1;
        this.n = this.n + 1;
        return this.n;
    }
}

function step(int seed): int {
    Counter c = new Counter(seed);
    return c.bumpTwice();
}

int total = 0;
for (int i = 0; i < 5; i = i + 1) {
    total = total + step(i);
}
print(total);
