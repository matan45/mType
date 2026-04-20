// MYT-199: LOAD_LOCAL_FLOAT / STORE_LOCAL_FLOAT correctness under a stable
// monomorphic site. The float accumulator loop drives the opcodes through
// the FLOAT variant's fast path; output must match generic dispatch.

function tightFloat(int n): float {
    float acc = 0.0;
    for (int i = 0; i < n; i = i + 1) {
        acc = acc + 0.5;
    }
    return acc;
}

print(tightFloat(1001));
