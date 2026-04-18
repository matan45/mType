// MYT-163 F-a: primary acceptance benchmark for speculative JIT inlining.
// Single-class compute() hot loop — the MONO IC site should inline away the
// call entirely once the caller is JIT-compiled. Same iteration count as
// method_dispatch.mt for easy comparison against the MYT-161 baseline.

class Shape {
    public function compute(int x): int {
        return x * 2 + 1;
    }
}

Shape s = new Shape();

int iterations = 2000000;
int acc = 0;
for (int i = 0; i < iterations; i = i + 1) {
    acc = acc + s.compute(i);
}

print("inline_monomorphic acc=" + acc);
