// MYT-300 regression: with JIT enabled, two patterns from rts_step2.mt
// misbehave (they work under --no-jit):
//
//   1. Selection-store — chained float `&&` stored into a bool field of an
//      object-array element. JIT path: bool field always becomes `true`.
//
//        units[i].selected = (ux >= minX && ux <= maxX
//                              && uy >= minY && uy <= maxY);
//
//   2. Edge-pan guard — chained float `&&` inside an `if` condition with
//      an `(float)int` cast on one operand. JIT path: branch never fires.
//
//        if (mx >= 0.0 && my >= 0.0 && mx < (float)WIN_W && my < (float)WIN_H)
//
// Each scenario runs ≥ 2000 outer iterations to cross the JIT hot threshold
// and the OSR tier-up so the bytecode actually executes JIT-compiled.
//
// Object array sized at 32 so the array uses SoA storage (the demo notes
// object arrays ≥ 16 elements switch to SoA).

class Unit {
    public float x;
    public float y;
    public bool  selected;

    public constructor(float xx, float yy) {
        this.x = xx;
        this.y = yy;
        this.selected = false;
    }
}

// --------------------------------------------------------------------
// Scenario A: selection-store. The chained float `&&` materializes a
// bool that gets written into Unit.selected. Only units with both
// x in [5,10] and y in [10,20] should end up selected. With layout
// (x = i, y = 2*i) the intersecting indices are i in {5,6,7,8,9,10}.
// Expected selectedCount = 6. Broken JIT: 32 (all units).
// --------------------------------------------------------------------
int N = 32;
Unit[] units = new Unit[N];
for (int i = 0; i < N; i = i + 1) {
    units[i] = new Unit((float)i, (float)(i * 2));
}

float minX = 5.0;
float maxX = 10.0;
float minY = 10.0;
float maxY = 20.0;

int hotIters = 2000;
int selectedCount = 0;
for (int it = 0; it < hotIters; it = it + 1) {
    for (int i = 0; i < N; i = i + 1) {
        float ux = units[i].x;
        float uy = units[i].y;
        units[i].selected = (ux >= minX && ux <= maxX
                              && uy >= minY && uy <= maxY);
    }
    // Count on the final iteration only — input is constant across iters
    // so the answer is the same every time, but counting once keeps the
    // running totals deterministic and small.
    if (it == hotIters - 1) {
        for (int i = 0; i < N; i = i + 1) {
            if (units[i].selected) { selectedCount = selectedCount + 1; }
        }
    }
}

// --------------------------------------------------------------------
// Scenario B: edge-pan guard. The chained float `&&` controls an if
// branch, and one term inside the chain is `(float)int`. We sweep a
// 5x3 grid of integer mouse-like positions per iter and count how many
// land "inside" the window, plus how many also satisfy the left-edge
// trigger. Expected per outer iter: inside=3, leftEdge=2, total=15.
// Over 2000 iters: inside=6000, leftEdge=4000, total=30000.
// Broken JIT: inside=0, leftEdge=0 (guard never fires).
// --------------------------------------------------------------------
int W = 800;
int H = 600;
int edgePad = 40;

int[] sampleX = new int[5];
sampleX[0] = -10;
sampleX[1] = 0;
sampleX[2] = 20;
sampleX[3] = 790;
sampleX[4] = 810;

int[] sampleY = new int[3];
sampleY[0] = -5;
sampleY[1] = 300;
sampleY[2] = 610;

int insideCount   = 0;
int leftEdgeCount = 0;
int totalSamples  = 0;

for (int it = 0; it < 2000; it = it + 1) {
    for (int i = 0; i < 5; i = i + 1) {
        for (int j = 0; j < 3; j = j + 1) {
            float mx = (float)sampleX[i];
            float my = (float)sampleY[j];
            totalSamples = totalSamples + 1;
            if (mx >= 0.0 && my >= 0.0 && mx < (float)W && my < (float)H) {
                insideCount = insideCount + 1;
                if (mx < (float)edgePad) { leftEdgeCount = leftEdgeCount + 1; }
            }
        }
    }
}

print("selectedCount=" + selectedCount);
print("insideCount="   + insideCount);
print("leftEdgeCount=" + leftEdgeCount);
print("totalSamples="  + totalSamples);
