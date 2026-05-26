// MYT-368 regression fixture. Mirrors the shape of
// C:\matan\RTSDemo\scripts\game\RTSHUDController.mt: an @Script-annotated
// class with 14 declared int fields and a constructor that assigns each
// of them to -1 by literal statement. With external imports stripped.
//
// Before MYT-368, peephole could shrink this body by ~13 instructions
// without decrementing FunctionMetadata.instructionCount, leaving the
// recorded range stale. Downstream readers (JIT codegen scans,
// ExceptionHandler search bounds, direct-call size gate) walked into the
// next function. In VertexForge that surfaced as cmdHoldId / cmdBuildId
// reading 0 instead of -1 and the host engine aborting.
//
// Test 4 ("@Script 14-field ctor: all field assignments execute (MYT-368)")
// instantiates this class and prints every field; the .expected file pins
// all 14 to -1.

@Script
class RTSHudStyleManyFields {
    public int f0;
    public int f1;
    public int f2;
    public int f3;
    public int f4;
    public int f5;
    public int f6;
    public int f7;
    public int f8;
    public int f9;
    public int f10;
    public int f11;
    public int f12;
    public int f13;

    constructor() {
        this.f0 = -1;
        this.f1 = -1;
        this.f2 = -1;
        this.f3 = -1;
        this.f4 = -1;
        this.f5 = -1;
        this.f6 = -1;
        this.f7 = -1;
        this.f8 = -1;
        this.f9 = -1;
        this.f10 = -1;
        this.f11 = -1;
        this.f12 = -1;
        this.f13 = -1;
    }

    // Sibling methods so <init> lands at a non-trivial startOffset and
    // peephole has nearby context to apply patterns to.
    public function onStart(): void {
        int a = 1 + 2;
        int b = 3 * 4;
        int c = a + b;
    }

    public function onUpdate(float dt): void {
        int x = 5 + 6;
        int y = 7 * 8;
        int z = x + y;
    }

    public function onDestroy(): void {
    }

    private function seedWidgets(): void {
        int p = 9 + 10;
        int q = 11 * 12;
        int r = p + q;
    }
}

RTSHudStyleManyFields c = new RTSHudStyleManyFields();
print(c.f0);
print(c.f1);
print(c.f2);
print(c.f3);
print(c.f4);
print(c.f5);
print(c.f6);
print(c.f7);
print(c.f8);
print(c.f9);
print(c.f10);
print(c.f11);
print(c.f12);
print(c.f13);
