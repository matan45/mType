// MYT-199: LOAD_LOCAL_INT / STORE_LOCAL_INT correctness under a stable
// monomorphic site. Runs enough iterations that the generic LOAD_LOCAL /
// STORE_LOCAL opcodes inside tightInt promote to the INT-specialized
// variants. Output must match the pre-rewrite dispatch.

function tightInt(int n): int {
    int acc = 0;
    for (int i = 0; i < n; i = i + 1) {
        acc = acc + i * 2;
    }
    return acc;
}

print(tightInt(1000));
