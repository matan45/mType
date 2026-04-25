// MYT-203: CALL_METHOD_POLY_CACHED promotion across MONO->POLY transitions.
// Warmup with one shape stabilizes to MONO -> CALL_METHOD_CACHED. Adding a
// second shape triggers CACHED deopt (cachedDeoptCount=1) but
// polyCachedDeoptCount stays at 0, so the IC populate path then re-promotes
// the opcode to CALL_METHOD_POLY_CACHED with 2 snapshot entries. A third
// shape arrival idempotently re-snapshots all 3 entries; the opcode stays
// POLY_CACHED. Output is verified analytically below.

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

class ShapeC extends Base {
    @Override
    public function compute(int x): int {
        return x * 5 + 11;
    }
}

function run(Base s, int x): int {
    return s.compute(x);
}

ShapeA a = new ShapeA();
ShapeB b = new ShapeB();
ShapeC c = new ShapeC();
int acc = 0;

// Phase 1: 200 ShapeA calls. IC: MONO. Opcode: CALL_METHOD_CACHED.
// sum(2*i + 1, i=0..199) = 2*19900 + 200 = 40000
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + run(a, i);
}

// Phase 2: 200 ShapeB calls. First call: CACHED shape miss ->
// deoptAndReprocess (cachedDeoptCount=1). IC adds B: state MONO->POLY.
// tryPromoteToPolyCached fires (polyCachedDeoptCount=0) -> opcode is
// rewritten to CALL_METHOD_POLY_CACHED with 2 entries.
// sum(3*i + 7, i=0..199) = 59700 + 1400 = 61100
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + run(b, i);
}

// Phase 3: 200 ShapeC calls. POLY_CACHED linear-scan misses (snapshot has
// A and B). handleCallMethodIC slow-path adds C: state stays POLY
// (entryCount=3). tryPromoteToPolyCached re-snapshots; opcode stays
// POLY_CACHED.
// sum(5*i + 11, i=0..199) = 99500 + 2200 = 101700
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + run(c, i);
}

// Phase 4: 100 interleaved A/B/C calls. POLY_CACHED hits on each shape via
// linear scan; no deopt, no MEGA. Verifies steady-state correctness with
// 3 active entries.
// sum((2*i+1) + (3*i+7) + (5*i+11), i=0..99) = sum(10*i + 19) = 49500 + 1900 = 51400
for (int i = 0; i < 100; i = i + 1) {
    acc = acc + run(a, i);
    acc = acc + run(b, i);
    acc = acc + run(c, i);
}

// Total: 40000 + 61100 + 101700 + 51400 = 254200
print(acc);
