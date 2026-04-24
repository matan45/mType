// MYT-210: primary acceptance benchmark for plain-CALL JIT inlining.
//
// distanceSq is a small leaf function (3 ops: subtract, subtract, mul-mul-add).
// Without inlining each iteration pays ~50-100 ns for the jit_call_function
// helper invoke + frame push/pop. With the inliner live the call collapses
// into the OSR-compiled outer loop body and constants flow through.
//
// Run with: mType --benchmark=tests/testFiles/benchmarks/function_call_hot.mt --jit-stats
// Baseline: revert the inliner commit (or run --no-jit, with the caveat
// that this also removes all other JIT speedups).
// Acceptance: "Inline decisions (function): INLINE" counter > 0; wall-time
// drops measurably vs the pre-inliner baseline.

function distanceSq(int x1, int y1, int x2, int y2): int {
    int dx = x1 - x2;
    int dy = y1 - y2;
    return dx*dx + dy*dy;
}

int N = 2000000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    total = total + distanceSq(i, i+1, i+2, i+3);
}
print("function_call_hot total=" + total);
