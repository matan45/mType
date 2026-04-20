// MYT-173: CALL_METHOD_CACHED deopt correctness. Warmup with one shape
// stabilizes the IC to MONO and promotes the opcode to CALL_METHOD_CACHED.
// The subsequent shape switch must trigger deopt — opcode rewrites back to
// CALL_METHOD, cachedDeoptCount becomes sticky, and the IC observes the
// new shape (transitioning MONO -> POLY). Interleaved calls afterwards
// must all produce correct results without ping-ponging.

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

function run(Base s, int x): int {
    return s.compute(x);
}

ShapeA a = new ShapeA();
ShapeB b = new ShapeB();
int acc = 0;

// Warmup MONO (ShapeA only).
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + run(a, i);
}

// Shape switch — first call triggers deopt from CACHED back to CALL_METHOD.
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + run(b, i);
}

// Interleaved POLY — verify stability post-deopt.
for (int i = 0; i < 100; i = i + 1) {
    acc = acc + run(a, i);
    acc = acc + run(b, i);
}

print(acc);
