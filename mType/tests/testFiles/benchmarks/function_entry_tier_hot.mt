// MYT-314: hot non-looping leaf-methods tier up via the function-entry path.
//
// Three independent leaves, each called N times from the outer loop body.
// No leaf has an internal loop, so OSR cannot see them; the plain-function
// inliner is currently disabled (see InlineDecisionCounters note in
// JitCompiler.hpp), so the only way these leaves become native code is the
// MYT-314 function-entry tier: PROFILE_ENTER -> recordEntry past 100 ->
// JitCompiler::compile -> jitCodeCache install -> subsequent calls dispatch
// via interpreter -> JIT (executeCallWithJit) or via the JIT outer loop's
// jit_call_function helper.
//
// Run with: mType --benchmark=tests/testFiles/benchmarks/function_entry_tier_hot.mt --jit-stats
// Baseline: same script with --no-jit. JIT-on wall-time should be measurably
// lower, and --jit-stats should show non-zero compileCount entries for add3,
// mul2, combine.

function add3(int x): int {
    return x + 3;
}

function mul2(int x): int {
    return x * 2;
}

function combine(int a, int b): int {
    return a + b;
}

int N = 2000000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    total = total + combine(add3(i), mul2(i));
}
print("function_entry_tier_hot total=" + total);
