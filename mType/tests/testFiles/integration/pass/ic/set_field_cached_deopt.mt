// MYT-194: SET_FIELD_CACHED deopt correctness. Warmup with one shape
// stabilizes the field IC to MONO and promotes SET_FIELD -> SET_FIELD_CACHED.
// The subsequent shape switch must trigger deopt — opcode rewrites back to
// SET_FIELD, cachedFieldDeoptCount becomes sticky, and the IC observes the
// new shape. Post-deopt writes from both shapes must land correctly.

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

function writeValue(Base b, int v): void {
    b.value = v;
}

ShapeA a = new ShapeA();
ShapeB b = new ShapeB();

// Warmup MONO (ShapeA only) — promotes SET_FIELD -> SET_FIELD_CACHED.
for (int i = 0; i < 200; i = i + 1) {
    writeValue(a, i);
}
print(a.value);

// Shape switch — first call triggers deopt.
for (int i = 0; i < 200; i = i + 1) {
    writeValue(b, i);
}
print(b.value);

// Interleaved POLY — verify both shapes still write correctly.
for (int i = 0; i < 100; i = i + 1) {
    writeValue(a, i + 1000);
    writeValue(b, i + 2000);
}
print(a.value);
print(b.value);
