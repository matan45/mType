// TARGET: ~2s on dev machine. Adjust N if first run lands outside 1-5s.
// Exercises short-circuit && / || chains inside a hot loop.
// Baseline coverage for MYT-142 (JIT emission for short-circuit jumps).

int N = 5000000;
int hits = 0;
for (int i = 0; i < N; i = i + 1) {
    bool a = (i % 3 == 0) && (i % 5 != 0) && (i > 0);
    bool b = (i % 7 == 0) || (i % 11 == 0) || (i % 13 == 0);
    if (a && b) {
        hits = hits + 1;
    } else if (a || b) {
        hits = hits + 2;
    }
}

print("short_circuit_chain hits=" + hits);
