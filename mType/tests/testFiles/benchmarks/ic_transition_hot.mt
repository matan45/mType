// TARGET: ~1-3s on dev machine. Adjust P if first run lands outside that range.
// Exercises IC state transitions at a SINGLE call site. The CALL_METHOD inside
// run() is shared across three phases:
//   phase1 (3 entries of class A)     -> MONOMORPHIC
//   phase2 (cycles A,B,C,D)           -> POLYMORPHIC (fills the POLY-4 width)
//   phase3 (cycles A..G)              -> MEGAMORPHIC (overflows the IC)
// Without a wider mega tier, phase3 pays the full runtime-lookup cost for
// every dispatch. P=420000 is a common multiple of 3, 4, and 7 so each phase
// completes whole cycles.

class Shape {
    public function compute(int x): int { return x; }
}

class A extends Shape {
    @Override public function compute(int x): int { return x + 1; }
}

class B extends Shape {
    @Override public function compute(int x): int { return x + 2; }
}

class C extends Shape {
    @Override public function compute(int x): int { return x + 3; }
}

class D extends Shape {
    @Override public function compute(int x): int { return x + 4; }
}

class E extends Shape {
    @Override public function compute(int x): int { return x + 5; }
}

class F extends Shape {
    @Override public function compute(int x): int { return x + 6; }
}

class G extends Shape {
    @Override public function compute(int x): int { return x + 7; }
}

function run(Shape[] arr, int len, int n): int {
    int acc = 0;
    for (int i = 0; i < n; i = i + 1) {
        acc = acc + arr[i % len].compute(i);
    }
    return acc;
}

Shape[] phase1 = [new A(), new A(), new A()];
Shape[] phase2 = [new A(), new B(), new C(), new D()];
Shape[] phase3 = [new A(), new B(), new C(), new D(), new E(), new F(), new G()];

int P = 420000;
int total = 0;
total = total + run(phase1, 3, P);
total = total + run(phase2, 4, P);
total = total + run(phase3, 7, P);

print("ic_transition_hot total=" + total);
