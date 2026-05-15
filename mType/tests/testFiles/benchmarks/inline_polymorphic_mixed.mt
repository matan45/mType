// MYT-173 follow-up: mixed-inlineability POLY site. Three subclasses (A, B,
// C) have one-line bodies that fit INLINE_SIZE_LIMIT (32 ops); the fourth
// (Big) is intentionally over the per-entry cap. Pre-change every dispatch
// ran the IC helper because the all-or-nothing eligibility gate rejected
// the entire site as soon as Big's entry hit CALLEE_TOO_BIG. Post-change
// A/B/C inline directly, Big routes through emitCallMethodOpGeneric per-
// shape, and the guard chain is identical to a homogeneous-inlineability
// POLY site.
//
// Primary regression benchmark for InlineAnalysis::checkInlineEligibility
// per-entry decisions + emitInlinedMethodCallPoly per-shape helper routing.
// Companion to inline_polymorphic.mt (all-inlineable POLY-4) and
// megamorphic_dispatch.mt (5+ shapes, MEGA cliff).

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

// Big's body is intentionally larger than INLINE_SIZE_LIMIT (32 ops) so its
// per-entry eligibility check fails with CALLEE_TOO_BIG. The JIT routes
// dispatches that hit Big through jit_call_method_ic while A/B/C inline.
// Thirty straight-line increments of small distinct constants keep the
// bytecode size well above the cap (≈63 ops post-fusion) while keeping
// `acc` within int64 — Big.compute(x) = x + 465, and the total accumulator
// stays around 2e12, well below INT64_MAX. Distinct constants prevent
// AST-level constant folding from collapsing the chain.
class Big extends Shape {
    @Override
    public function compute(int x): int {
        int r = x;
        r = r + 1;
        r = r + 2;
        r = r + 3;
        r = r + 4;
        r = r + 5;
        r = r + 6;
        r = r + 7;
        r = r + 8;
        r = r + 9;
        r = r + 10;
        r = r + 11;
        r = r + 12;
        r = r + 13;
        r = r + 14;
        r = r + 15;
        r = r + 16;
        r = r + 17;
        r = r + 18;
        r = r + 19;
        r = r + 20;
        r = r + 21;
        r = r + 22;
        r = r + 23;
        r = r + 24;
        r = r + 25;
        r = r + 26;
        r = r + 27;
        r = r + 28;
        r = r + 29;
        r = r + 30;
        return r;
    }
}

Shape[] shapes = [new A(), new B(), new C(), new Big()];

int iterations = 2000000;
int acc = 0;
for (int i = 0; i < iterations; i = i + 1) {
    acc = acc + shapes[i % 4].compute(i);
}

print("inline_polymorphic_mixed acc=" + acc);
