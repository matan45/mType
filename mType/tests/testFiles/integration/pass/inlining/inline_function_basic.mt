// MYT-210: trivial plain-CALL inlining. The function body is 3 ops
// (LOAD_LOCAL, MUL_INT constant, RETURN_VALUE) — inside INLINE_SIZE_LIMIT
// with no internal jumps. Output must match the non-inlined path.

function doubleIt(int x): int {
    return x * 2;
}

print(doubleIt(5));
print(doubleIt(10));
print(doubleIt(21));
