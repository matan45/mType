// MYT-210: depth-2 inlining test. f calls g; both small leaves. Verifies
// that the recursion guard (per-callee name compare against currentCompilingFn)
// does NOT misfire on legitimate cross-function calls, and that the
// HAS_NESTED_CALL gate in scanCalleeOpcodes correctly bails when f's body
// contains a CALL/CALL_FAST to g (so f stays out-of-line, not inlined).

function inner(int x): int {
    return x + 1;
}

function outer(int x): int {
    return inner(x) * 3;
}

print(outer(2));   // (2+1)*3 = 9
print(outer(7));   // (7+1)*3 = 24
print(outer(13));  // (13+1)*3 = 42
