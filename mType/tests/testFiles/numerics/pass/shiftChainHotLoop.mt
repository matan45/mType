// Test: shift/mask chains inside a hot loop (crosses the OSR threshold so the
// JIT pass exercises the JIT shift helpers; --no-jit pass exercises the
// interpreter). Self-verifying: the accumulator must come back to 0.
function shiftRoundTrip(int n): int {
    int acc = 0;
    for (int i = 0; i < n; i++) {
        acc = acc + (((i << 3) >> 3) - i);
        acc = acc + ((i & 7) ^ (i & 7));
    }
    return acc;
}

function main(): void {
    print(shiftRoundTrip(20000));
    int x = 1;
    for (int i = 0; i < 64; i++) {
        x = x << 1;
    }
    print(x);
}
main();

// Expected output:
// 0
// 0
