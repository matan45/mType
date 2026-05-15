// MYT-322: acceptance benchmark for free-function direct JIT-to-JIT
// dispatch. Pairs with the four existing function-call benchmarks
// (function_call_hot, function_entry_tier_hot, generic_dispatch_hot,
// static_call_hot), all of which have callees ≤ 4 bytecode instructions
// — by design, those stay below MIN_DIRECT_CALL_INSTRUCTION_COUNT (15)
// and exercise the mini-interpret fallback only.
//
// This benchmark fills the missing canary: a free-function callee
// whose body is comfortably above the size gate (~30 instructions
// from 6 binary-op assignments plus a 5-way return sum), called from
// a hot outer loop. Once function-entry tiering JIT-compiles
// `mediumCompute` (after ~100 calls), the lazy refresh in
// jit_call_function_ic's warm path picks up the freshly-compiled
// pointer and the rest of the loop routes through
// jit_call_function_direct.
//
// Run with: mType --benchmark=tests/testFiles/benchmarks/function_call_medium_hot.mt --jit-stats
// Baseline: same script with --no-jit, or pre-MYT-322 (warm path
// falls through to callFunctionFromJitDirect's mini-interpret on
// every iteration even though the callee is JIT-compiled).
// Acceptance: wall-time drops measurably under jit-on vs baseline;
// --jit-stats shows mediumCompute in compileCount.

function mediumCompute(int x): int {
    int a = x + 1;
    int b = x + 2;
    int c = x * 3;
    int d = x - 1;
    int e = c + a;
    int f = c - b;
    int g = e + d;
    int h = f * 2;
    return a + b + c + d + e + f + g + h;
}

int N = 2000000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    total = total + mediumCompute(i);
}
print("function_call_medium_hot total=" + total);
