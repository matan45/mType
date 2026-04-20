// MYT-198: shape change after fusion forces un-fuse + sticky demote.
// Warmup with ShapeA stabilizes the IC to MONO and promotes to the fused
// LOAD_LOCAL_CALL_CACHED. The switch to ShapeB triggers shape-miss deopt;
// deoptAndReprocess must un-fuse first (restore LOAD_LOCAL + demote opcode
// to CALL_METHOD) before the generic IC re-observes the new shape. Final
// interleaved pass validates stability — no ping-pong back to fused.

class Base {
    public function compute(int x): int { return x; }
}

class ShapeA extends Base {
    @Override
    public function compute(int x): int { return x + 1; }
}

class ShapeB extends Base {
    @Override
    public function compute(int x): int { return x * 2 + 5; }
}

function run(Base s, int x): int {
    return s.compute(x);
}

ShapeA a = new ShapeA();
ShapeB b = new ShapeB();
int acc = 0;

// Warmup — stabilize MONO and trigger fusion.
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + run(a, i);
}

// Shape switch — fusion must un-fuse before the CACHED demote.
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + run(b, i);
}

// Interleaved POLY — no re-fusion, no ping-pong.
for (int i = 0; i < 100; i = i + 1) {
    acc = acc + run(a, i);
    acc = acc + run(b, i);
}

print(acc);
