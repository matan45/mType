// MYT-165 F-c: polymorphic inlining correctness test. Four subclasses of
// Shape each override compute(int); the call site sees all four at runtime,
// driving the inline cache to POLYMORPHIC with entryCount == 4 (fills the
// IC_MAX_POLYMORPHIC_ENTRIES = 4 budget exactly). The JIT emits a chained-
// guard layout: extract classDef once, cmp/jne through shape 0..3, inlined
// bodies on fall-through, shared slow-path helper on guard-chain miss.

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

int acc = 0;
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + shapes[i % 4].compute(i);
}
print(acc);
