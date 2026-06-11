// Test: INT64_MIN / -1 is defined as INT64_MIN (Java semantics), not a
// hardware idiv overflow trap.
function main(): void {
    int min = 1 << 63;
    print(min / -1);
}
main();

// Expected output:
// -9223372036854775808
