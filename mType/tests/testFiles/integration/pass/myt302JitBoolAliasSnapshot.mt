// MYT-302 regression: under JIT, `Unit u = arr[i]; if (u.selected) { ... }` on a
// SoA-backed object array (>= 16 elements) silently exits with no output — the
// trailing print never reaches stdout. With --no-jit the script completes. The
// alias-snapshot path goes through ObjectArrayBase::materializeInstance, which
// allocates a fresh ObjectInstance per iter; a throw from that path unwinds
// through the asmjit-generated frame for jit_array_get and the OS terminates
// the process (no PE x64 unwind data registered — same hazard MYT-268 fixed
// for OSRDeoptException by switching to the pendingException stash convention).
//
// Reproduction needs:
//   1. Object array size >= 16 → SoA storage (NativeArray uses fieldArrays_).
//   2. Hot loop with the SNAPSHOT pattern `Unit u = arr[i]` then field-read
//      (the direct `arr[i].field` pattern was MYT-300 and is already fixed).
//   3. Iteration count > OSR threshold so the JIT actually executes the body.
//   4. A bool field read in an `if` guard — this is the failing site; pair
//      with a float field read on the same snapshot so a regression of MYT-300
//      also fires here.
//
// Distinct from MYT-300 (fused ARRAY_GET_FIELD on direct `arr[i].field`) and
// MYT-292 (bool local after a call return).

class Unit {
    public float x;
    public bool selected;
    public constructor(float xx, bool sel) {
        this.x = xx;
        this.selected = sel;
    }
}

int N = 32;                                // >= 16 → SoA storage
Unit[] units = new Unit[N];
int trueIdxLimit = 5;                      // first 5 selected=true, rest false
for (int i = 0; i < N; i = i + 1) {
    units[i] = new Unit((float)i, i < trueIdxLimit);
}

int trueSightings = 0;
int falseSightings = 0;
float xSum = 0.0;

for (int frame = 0; frame < 2000; frame = frame + 1) {
    // Loop 1: bool alias-read in if-guard (the failing site).
    for (int i = 0; i < N; i = i + 1) {
        Unit u = units[i];                 // snapshot via materializeInstance
        if (u.selected) {
            trueSightings = trueSightings + 1;
        } else {
            falseSightings = falseSightings + 1;
        }
    }
    // Loop 2: float alias-read on the snapshot (MYT-300 regression rerun on
    // the snapshot variant rather than the fused ARRAY_GET_FIELD path).
    for (int i = 0; i < N; i = i + 1) {
        Unit u = units[i];
        xSum = xSum + u.x;
    }
}

print("trueSightings=" + trueSightings);
print("falseSightings=" + falseSightings);
print("xSum=" + xSum);
