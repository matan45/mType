// MYT-210: self-recursive function must not inline. checkCalleeEligibility's
// SELF_RECURSIVE guard must fire on the recursive site (also caught earlier
// by HAS_NESTED_CALL since fib's body contains a CALL to fib). Output must
// match the non-inlined path; bad inlining would either overflow or
// miscount.

function fib(int n): int {
    if (n < 2) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

print(fib(0));   // 0
print(fib(1));   // 1
print(fib(10));  // 55
print(fib(15));  // 610
