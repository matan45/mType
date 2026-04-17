// MYT-153 Bug #1 regression: nested loop OSR.
//
// Outer and inner back-edges both cross the 500-iteration tier-up
// threshold, so OSR compiles both levels. A non-balanced
// findLoopBoundaries picks the inner LOOP_END for the outer back-edge,
// clipping the compile range and silently degenerating the outer loop
// into one iteration — returning only the first inner row.
//
// Outer i = 0..599, inner j = 0..9, value added = i*10 + j.
//   sum = Σ_i Σ_j (i*10 + j)
//       = Σ_i (100*i + 45)
//       = 100 * (599*600/2) + 600*45
//       = 17_970_000 + 27_000
//       = 17_997_000

function nestedSum(int outer, int inner): int {
    int sum = 0;
    for (int i = 0; i < outer; i = i + 1) {
        for (int j = 0; j < inner; j = j + 1) {
            sum = sum + (i * inner + j);
        }
    }
    return sum;
}

print("nestedSum(600, 10): " + nestedSum(600, 10));
