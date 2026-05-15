// MYT-314 regression: a hot non-looping leaf function must JIT-compile via
// the function-entry tier path (PROFILE_ENTER -> JitProfiler::recordEntry
// hits 100-entry threshold -> JitCompiler::compile -> JitCodeCache install).
// Pre-MYT-314 this only worked through OSR, so a leaf with no internal loop
// could never tier up regardless of how many times it was called.
//
// `leaf` is a tail-style recursion that crosses the 100-entry threshold
// during the descent. Correctness (sum 1..150 = 11325) is the suite-level
// guard; the actual JIT pathway is verified manually via --jit-stats
// (compileCount for `leaf` >= 1, bailoutCount unchanged for `leaf`).
//
// The integration suite runs both with JIT on and with --no-jit, so this
// fixture also exercises AC item 6 (no-jit mode still works).

function leaf(int n, int acc): int {
    if (n == 0) return acc;
    return leaf(n - 1, acc + n);
}

int total = leaf(150, 0);
print("myt314 total=" + total);
