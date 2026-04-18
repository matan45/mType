// MYT-165 F-c: primary acceptance benchmark for polymorphic chained-guard
// inlining. Four subclasses fill IC_MAX_POLYMORPHIC_ENTRIES exactly, driving
// the JIT'd caller to emit a 4-way shape-guard chain with inlined bodies per
// shape. Same iteration count as inline_monomorphic.mt / inline_branching.mt
// for side-by-side comparison with MONO and branching inlining.
//
// Pairs with method_dispatch.mt (3 subclasses, N=3 chain) — together they
// cover the common polymorphic case and the maximum-IC-width case.

class Shape {
    public function compute(int x): int {
        return x;
    }
}

class A extends Shape {
    @Override
    public function compute(int x): int {
        return x + 10;
    }
}

class B extends Shape {
    @Override
    public function compute(int x): int {
        return x + 20;
    }
}

class C extends Shape {
    @Override
    public function compute(int x): int {
        return x + 30;
    }
}

class D extends Shape {
    @Override
    public function compute(int x): int {
        return x + 40;
    }
}

Shape[] shapes = [new A(), new B(), new C(), new D()];

int iterations = 2000000;
int acc = 0;
for (int i = 0; i < iterations; i = i + 1) {
    acc = acc + shapes[i % 4].compute(i);
}

print("inline_polymorphic acc=" + acc);
