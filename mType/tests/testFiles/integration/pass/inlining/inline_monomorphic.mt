// MYT-163 F-a: primary correctness mirror of the benchmark (same shape,
// smaller iteration count). Single-class `compute` hot loop — MONO call
// site eligible for inlining.

class Shape {
    public function compute(int x): int {
        return x * 2 + 1;
    }
}

Shape s = new Shape();
int acc = 0;
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + s.compute(i);
}
print(acc);
