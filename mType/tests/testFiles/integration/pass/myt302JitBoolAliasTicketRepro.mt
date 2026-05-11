// MYT-302 ticket repro (verbatim from the bug report at MYT-302). The
// myt302JitBoolAliasSnapshot.mt sibling covers both `if`-branches with a
// 32-element array to surface the wrong-result shape of this bug; this
// file pins the exact pattern reported in the ticket — 512-element array
// with only 20 elements initialized, all selected=false, so the expected
// trueSightings = 0. Under the original report the JIT path printed no
// output at all (silent exit); the wrong-result variant is the more
// likely shape and matches the snapshot test, but keeping the ticket
// repro in the suite guards against a future regression to the
// silent-exit shape too.

class Unit {
    public float x;
    public bool selected;
    public constructor(float x) { this.x = x; this.selected = false; }
}

// Simulates a "draw" native — opaque side effect inside the then-branch.
function consume(float x): void { }

Unit[] units = new Unit[512];
int n = 20;
for (int i = 0; i < n; i++) { units[i] = new Unit(i * 1.0); }

int trueSightings = 0;
for (int frame = 0; frame < 5000; frame++) {
    // Mirror the game: two alias-read loops back-to-back over the same array.
    for (int i = 0; i < n; i++) {
        Unit u = units[i];
        if (u.selected) {            // JIT-suspect: alias-read of SoA bool
            consume(u.x);
            trueSightings = trueSightings + 1;
        }
    }
    for (int i = 0; i < n; i++) {
        Unit u = units[i];
        consume(u.x);                // alias-read of SoA float (works)
    }
}
print("trueSightings=" + trueSightings + " (expected 0)");
