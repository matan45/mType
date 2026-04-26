// Closure capture read in a hot loop. The lambda body reads a captured local
// every call — isolates the captured-variable indirection cost vs a non-capturing
// lambda (compare against lambda_call_hot.mt).

interface IntFunction { function apply(int x): int; }

function build(int captured): IntFunction {
    return x -> x * captured + captured;
}

function run(IntFunction fn, int n): int {
    int total = 0;
    for (int i = 0; i < n; i = i + 1) {
        total = total + fn.apply(i);
    }
    return total;
}

IntFunction f = build(7);
int N = 2000000;
int total = run(f, N);
print("lambda_closure_hot total=" + total);
