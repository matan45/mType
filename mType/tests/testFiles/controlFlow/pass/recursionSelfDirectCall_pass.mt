// MYT-207 Phase 2: non-tail self-recursive direct-call lowering. The JIT
// detects CALL <self> in a non-tail position and emits a direct asmjit
// invoke against the function's own FuncNode->label(), skipping the
// jit_call_function dispatch overhead. This test pins down correctness of
// the fast path AND the depth-overflow fallback (when MAX_JIT_NATIVE_DEPTH
// is exceeded, emission falls back to the generic helper which itself
// bails to the interpreter).

// Classic non-tail recursion: result of f(n-1) is consumed by the +.
function fib(int n): int {
    if (n < 2) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

// Mixed tail + non-tail: ack(m-1, 1) is tail; ack(m-1, ack(m, n-1)) has
// inner non-tail and outer tail. Exercises both paths in one function.
function ack(int m, int n): int {
    if (m == 0) {
        return n + 1;
    }
    if (n == 0) {
        return ack(m - 1, 1);
    }
    return ack(m - 1, ack(m, n - 1));
}

// Linear non-tail recursion deeper than MAX_JIT_NATIVE_DEPTH (64). The
// direct-invoke path inlines a depth guard and falls back to the helper
// (and ultimately the interpreter) once depth is exceeded — this asserts
// the fallback returns the correct result.
function chain(int n): int {
    if (n <= 0) {
        return 0;
    }
    return chain(n - 1) + 1;
}

print("fib(10) = " + fib(10));        // 55
print("fib(15) = " + fib(15));        // 610
print("ack(2, 3) = " + ack(2, 3));    // 9
print("ack(3, 3) = " + ack(3, 3));    // 61
print("chain(80) = " + chain(80));    // 80 — exceeds 64-deep guard
print("Self direct call tests completed successfully");
