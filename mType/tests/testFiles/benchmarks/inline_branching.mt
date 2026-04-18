// MYT-164 F-b: primary acceptance benchmark for internal-jump inlining.
// Single-class compute() hot loop where the method body has a guard branch
// (mirrors the iterator hasNext / null-check pattern that F-a couldn't
// touch). F-a leaves this site un-inlined — it bails on HAS_INTERNAL_JUMPS
// at the first JUMP_IF_FALSE. F-b's per-frame localJumpLabels pre-scan
// lifts the restriction. Same iteration count as inline_monomorphic.mt
// for easy side-by-side comparison.

class Shape {
    public function compute(int x): int {
        if (x < 0) {
            return 0;
        }
        return x * 2 + 1;
    }
}

Shape s = new Shape();

int iterations = 2000000;
int acc = 0;
for (int i = 0; i < iterations; i = i + 1) {
    acc = acc + s.compute(i);
}

print("inline_branching acc=" + acc);
