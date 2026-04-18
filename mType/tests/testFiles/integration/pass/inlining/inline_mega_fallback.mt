// MYT-165 F-c: MEGA fallback regression. Five subclasses exceed
// IC_MAX_POLYMORPHIC_ENTRIES = 4, so the inline cache flips to MEGAMORPHIC
// after the fifth distinct receiver shape lands on the call site. The
// eligibility gate at InlineAnalysis.cpp rejects MEGA state
// (IC_NOT_MONOMORPHIC), so the JIT falls through to the existing
// emitCallMethodOpGeneric path — which routes through jit_call_method_ic
// and handles MEGA via its standard dispatch. Asserts that widening past
// POLY does not corrupt state or hang.

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

Shape[] shapes = [new A(), new B(), new C(), new D(), new E()];

int acc = 0;
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + shapes[i % 5].compute(i);
}
print(acc);
