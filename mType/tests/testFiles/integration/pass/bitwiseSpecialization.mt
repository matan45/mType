// Regression test for INT-specialized bitwise opcodes (BITWISE_AND_INT etc.)
// and trySpecializeBitwise runtime promotion. Each loop runs past the
// specialization threshold so trySpecializeBitwise rewrites the opcode
// to the INT variant at least once; the final sum verifies the specialized
// handler's semantics match the generic one bit-identically.
//
// Values picked so each running total is easy to verify by hand:
//   - (i & 1) alternates 0/1 → sum over 100 iters = 50
//   - (i | 0) = i          → sum over 10 iters = 45
//   - (i ^ 0) = i          → sum over 10 iters = 45
//   - (i << 1) = 2*i       → sum over 10 iters = 90
//   - (i >> 0) = i          → sum over 10 iters = 45
//   - ~(-1) = 0            → 100 iters, all 0 → 0

function main(): void {
    // BITWISE_AND_INT — 100 iters past threshold
    int accA = 0;
    int i = 0;
    while (i < 100) {
        accA = accA + (i & 1);
        i = i + 1;
    }
    print("and = " + accA);

    // BITWISE_OR_INT — identity with OR 0
    int accO = 0;
    int j = 0;
    while (j < 10) {
        accO = accO + (j | 0);
        j = j + 1;
    }
    print("or = " + accO);

    // BITWISE_XOR_INT — identity with XOR 0
    int accX = 0;
    int k = 0;
    while (k < 10) {
        accX = accX + (k ^ 0);
        k = k + 1;
    }
    print("xor = " + accX);

    // LEFT_SHIFT_INT — (k << 1) = 2k, sum = 2 * 0..9 = 90
    int accL = 0;
    int m = 0;
    while (m < 10) {
        accL = accL + (m << 1);
        m = m + 1;
    }
    print("shl = " + accL);

    // RIGHT_SHIFT_INT — (k >> 0) = k, sum = 0..9 = 45
    int accR = 0;
    int n = 0;
    while (n < 10) {
        accR = accR + (n >> 0);
        n = n + 1;
    }
    print("shr = " + accR);

    // BITWISE_NOT_INT — ~(-1) = 0, 100 iters past unary threshold
    int accN = 0;
    int p = 0;
    while (p < 100) {
        int neg = 0 - 1;
        accN = accN + ~neg;
        p = p + 1;
    }
    print("not = " + accN);
}
main();
