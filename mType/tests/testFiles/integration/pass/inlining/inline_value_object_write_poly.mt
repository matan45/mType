// MYT-346: polymorphic IC variant. Two value classes (CounterA, CounterB)
// share a `bumpAndGet` method shape; alternating receivers in a hot loop
// forces the method-IC to POLYMORPHIC state, exercising the per-arm
// materialisation path in emitInlinedMethodCallPoly. Each call still
// returns 1 (value-class temp materialise-and-discard semantics) so the
// total is exactly the iteration count.

value class CounterA {
    public int n;

    public constructor(int n) {
        this.n = n;
    }

    public function bumpAndGet(): int {
        this.n = this.n + 1;
        return this.n;
    }
}

value class CounterB {
    public int n;

    public constructor(int n) {
        this.n = n;
    }

    public function bumpAndGet(): int {
        this.n = this.n + 1;
        return this.n;
    }
}

CounterA a = new CounterA(0);
CounterB b = new CounterB(0);
int total = 0;
for (int i = 0; i < 1000; i = i + 1) {
    if (i % 2 == 0) {
        total = total + a.bumpAndGet();
    } else {
        total = total + b.bumpAndGet();
    }
}
print(total);
