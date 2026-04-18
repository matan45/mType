// MYT-168: regression test for JIT MONO->POLY transition at an inlined
// CALL_METHOD site. Warmup drives hotCaller past the 100-invocation hot
// threshold with one receiver shape (ShapeA), so the caller is JIT-compiled
// with its method-IC in MONOMORPHIC state. The transition phase then invokes
// the same call site with ShapeB, hitting the inlined Mono shape-guard slow
// path which must route through jit_call_method_ic with the correct
// CALL_METHOD bytecode offset (not the callee's last offset).
//
// This is a regression guard: the original bug is silent in stdout because
// jit_call_method_ic at a wrong IP still dispatches correctly via the IC
// fallback, so the arithmetic is unaffected. But future drifts in the
// snapshot/restore machinery (e.g. missing another field) would surface
// here as wrong output or a crash.

class Base {
    public function compute(int x): int {
        return x;
    }
}

class ShapeA extends Base {
    @Override
    public function compute(int x): int {
        return x * 2 + 1;
    }
}

class ShapeB extends Base {
    @Override
    public function compute(int x): int {
        return x * 3 + 7;
    }
}

function hotCaller(Base s, int x): int {
    return s.compute(x);
}

ShapeA a = new ShapeA();
ShapeB b = new ShapeB();

int acc = 0;

// Warmup (MONO): 150 calls with ShapeA. JIT compiles hotCaller on the
// 100th PROFILE_ENTER with the IC still monomorphic.
for (int i = 0; i < 150; i = i + 1) {
    acc = acc + hotCaller(a, i);
}

// Transition: 50 calls with ShapeB. JIT'd hotCaller takes the shape-guard
// miss and slow-paths via emitCallMethodOpGeneric.
for (int i = 0; i < 50; i = i + 1) {
    acc = acc + hotCaller(b, i);
}

// Interleave to stabilise POLY state and keep the site callable after
// the transition.
for (int i = 0; i < 50; i = i + 1) {
    acc = acc + hotCaller(a, i);
    acc = acc + hotCaller(b, i);
}

print(acc);
