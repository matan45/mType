// MYT-203: POLY -> MEGA transition forces CALL_METHOD_POLY_CACHED back to
// CALL_METHOD with sticky polyCachedDeoptCount. Warmup populates all 4 POLY
// snapshot entries; the 5th unique shape triggers addEntry() to fail in
// MethodInlineCache (entryCount==4), state transitions to MEGAMORPHIC, and
// the MEGA-detect block in handleCallMethodIC rewrites the opcode and bumps
// polyCachedDeoptCount. Subsequent dispatches go through the generic
// CALL_METHOD path (cache.state==MEGAMORPHIC short-circuits to
// objectExecutor->handleCallMethod). Output verified analytically.

class Base {
    public function compute(int x): int {
        return x;
    }
}

class ShapeA extends Base {
    @Override
    public function compute(int x): int { return x + 1; }
}

class ShapeB extends Base {
    @Override
    public function compute(int x): int { return x + 2; }
}

class ShapeC extends Base {
    @Override
    public function compute(int x): int { return x + 3; }
}

class ShapeD extends Base {
    @Override
    public function compute(int x): int { return x + 4; }
}

class ShapeE extends Base {
    @Override
    public function compute(int x): int { return x + 5; }
}

function run(Base s, int x): int {
    return s.compute(x);
}

ShapeA a = new ShapeA();
ShapeB b = new ShapeB();
ShapeC c = new ShapeC();
ShapeD d = new ShapeD();
ShapeE e = new ShapeE();
int acc = 0;

// Phase 1: ShapeA only, 100 iters. MONO -> CALL_METHOD_CACHED.
// sum(i+1, i=0..99) = 4950 + 100 = 5050
for (int i = 0; i < 100; i = i + 1) {
    acc = acc + run(a, i);
}

// Phase 2: ShapeA / ShapeB alternating, 100 outer iters (200 calls). First
// B call deopts CACHED -> CALL_METHOD; IC populates B; tryPromoteToPolyCached
// fires (state=POLY, polyCachedDeoptCount=0) -> CALL_METHOD_POLY_CACHED with
// 2 entries. Steady state from there.
// sum((i+1) + (i+2), i=0..99) = sum(2*i+3) = 9900 + 300 = 10200
for (int i = 0; i < 100; i = i + 1) {
    acc = acc + run(a, i);
    acc = acc + run(b, i);
}

// Phase 3: ShapeC only, 100 iters. POLY_CACHED scan misses; IC populates C
// (state stays POLY, entryCount=3); re-snapshot. Opcode stays POLY_CACHED.
// sum(i+3, i=0..99) = 4950 + 300 = 5250
for (int i = 0; i < 100; i = i + 1) {
    acc = acc + run(c, i);
}

// Phase 4: ShapeD only, 100 iters. Re-snapshot to entryCount=4. Opcode
// stays POLY_CACHED, snapshot is now full.
// sum(i+4, i=0..99) = 4950 + 400 = 5350
for (int i = 0; i < 100; i = i + 1) {
    acc = acc + run(d, i);
}

// Phase 5: ShapeE only, 100 iters. First E call: POLY_CACHED scan misses
// (snapshot has A,B,C,D); handleCallMethodIC's addEntry returns false ->
// MEGA-detect demotes opcode to CALL_METHOD, polyCachedDeoptCount=1, IC
// state=MEGAMORPHIC. Subsequent E calls go through generic CALL_METHOD
// (MEGA short-circuit).
// sum(i+5, i=0..99) = 4950 + 500 = 5450
for (int i = 0; i < 100; i = i + 1) {
    acc = acc + run(e, i);
}

// Phase 6: all 5 shapes interleaved, 50 outer iters (250 calls). Site is
// pinned to generic CALL_METHOD via sticky polyCachedDeoptCount. Verifies
// correctness on the MEGA fallback path.
// sum((i+1)+(i+2)+(i+3)+(i+4)+(i+5), i=0..49) = sum(5*i+15) = 6125 + 750 = 6875
for (int i = 0; i < 50; i = i + 1) {
    acc = acc + run(a, i);
    acc = acc + run(b, i);
    acc = acc + run(c, i);
    acc = acc + run(d, i);
    acc = acc + run(e, i);
}

// Total: 5050 + 10200 + 5250 + 5350 + 5450 + 6875 = 38175
print(acc);
