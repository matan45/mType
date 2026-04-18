// MYT-164 F-b: callee with internal if/else + early returns. Lifts F-a's
// HAS_INTERNAL_JUMPS restriction (JUMP_IF_FALSE for the two `if` guards,
// JUMP for the fall-through to a final `return`). The inliner pre-scans
// the callee range, allocates a local label per target IP, and reroutes
// JUMPs through the inline frame. Correctness check: output matches the
// interpreter regardless of whether the JIT fired.

class Clamp {
    public function clamp(int x): int {
        if (x < 0) {
            return 0;
        }
        if (x > 10) {
            return 10;
        }
        return x;
    }
}

Clamp c = new Clamp();
int total = 0;
for (int i = 0; i < 20; i = i + 1) {
    total = total + c.clamp(i - 5);
}
print(total);
