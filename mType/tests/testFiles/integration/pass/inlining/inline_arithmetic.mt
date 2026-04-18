// MYT-163 F-a: hot-loop arithmetic. The method is small enough for inlining,
// and the loop runs past the JIT hot threshold so the inliner should fire.
// Correctness check: the sum equals the closed-form result regardless of
// whether inlining was applied.

class Adder {
    public function add(int x): int {
        return x + 1;
    }
}

Adder a = new Adder();
int total = 0;
for (int i = 0; i < 1000; i = i + 1) {
    total = total + a.add(i);
}
print(total);
