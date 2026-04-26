// Lambda invocation in a hot loop. Mirror of function_call_hot.mt but routed
// through an expression lambda assigned to a single-method interface — measures
// interface-dispatch + lambda-frame overhead vs an inlined direct call.

interface IntFunction { function apply(int x): int; }

function run(IntFunction fn, int n): int {
    int total = 0;
    for (int i = 0; i < n; i = i + 1) {
        total = total + fn.apply(i);
    }
    return total;
}

IntFunction doubler = x -> x * 2 + 1;
int N = 2000000;
int total = run(doubler, N);
print("lambda_call_hot total=" + total);
