// MYT-251 acceptance criterion #4: stress test for nested method calls
// inside an OSR-compiled hot loop. The outer for-loop's back-edge crosses
// the OSR threshold, so the loop body tiers up and is JIT-compiled with
// speculative method inlining engaged. Outer.compute calls Inner.step
// twice — both sites should inline (depth-1) inside the OSR-emitted body.
// Correctness check: sum_{i=0..N-1}(2*(i+1)) = N*(N+1)
// = 500000 * 500001 = 250000500000.

class Inner {
    public function step(int x): int {
        return x + 1;
    }
}

class Outer {
    public Inner inner;
    public constructor() {
        this.inner = new Inner();
    }
    public function compute(int x): int {
        return this.inner.step(x) + this.inner.step(x);
    }
}

Outer o = new Outer();
int N = 500000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    total = total + o.compute(i);
}
print("inline_osr_stress total=" + total);
