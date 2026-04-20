// MYT-194: GET_FIELD_CACHED deopt correctness. Warmup with one shape
// stabilizes the field IC to MONO and promotes GET_FIELD -> GET_FIELD_CACHED.
// The subsequent shape switch must trigger deopt — opcode rewrites back to
// GET_FIELD, cachedFieldDeoptCount becomes sticky, and the IC observes the
// new shape (transitioning MONO -> POLY). Interleaved reads afterwards
// must all produce correct values without ping-ponging.

class Base {
    public int value;
    public constructor() {
        this.value = 0;
    }
}

class ShapeA extends Base {
    public constructor() : super() {
    }
}

class ShapeB extends Base {
    public constructor() : super() {
    }
}

function readValue(Base b): int {
    return b.value;
}

ShapeA a = new ShapeA();
ShapeB b = new ShapeB();
a.value = 10;
b.value = 20;

int acc = 0;

// Warmup MONO (ShapeA only) — promotes GET_FIELD -> GET_FIELD_CACHED.
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + readValue(a);
}

// Shape switch — first call triggers deopt from CACHED back to GET_FIELD.
for (int i = 0; i < 200; i = i + 1) {
    acc = acc + readValue(b);
}

// Interleaved POLY — verify stability post-deopt.
for (int i = 0; i < 100; i = i + 1) {
    acc = acc + readValue(a);
    acc = acc + readValue(b);
}

print(acc);
