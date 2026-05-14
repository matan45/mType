// MYT-173 follow-up: megamorphic dispatch benchmark. Six subclasses overflow
// IC_MAX_POLYMORPHIC_ENTRIES (= 4), driving the method IC to MEGAMORPHIC
// state. Every dispatch routes through jit_call_method_ic with no inlined
// fast path — the MEGA cliff.
//
// Pairs with inline_polymorphic.mt (4 shapes, fills POLY exactly) and
// method_dispatch.mt (3 shapes, well below POLY width) to triangulate the
// inline / POLY-inline / MEGA performance profile.

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

class E extends Shape {
    @Override
    public function compute(int x): int {
        return x + 50;
    }
}

class F extends Shape {
    @Override
    public function compute(int x): int {
        return x + 60;
    }
}

Shape[] shapes = [new A(), new B(), new C(), new D(), new E(), new F()];

int iterations = 2000000;
int acc = 0;
for (int i = 0; i < iterations; i = i + 1) {
    acc = acc + shapes[i % 6].compute(i);
}

print("megamorphic_dispatch acc=" + acc);
