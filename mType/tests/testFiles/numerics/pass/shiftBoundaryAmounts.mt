// Test: shifts at legal boundary amounts (0 and 63); >> is arithmetic
// (sign-extending). Amounts outside 0..63 are error fixtures.
function main(): void {
    print(5 << 0);
    print(1 << 62);
    print(1 << 63);
    print(16 >> 0);
    print(16 >> 2);
    print(-16 >> 2);
    print(-1 >> 63);
    int min = 1 << 63;
    print(min >> 63);
}
main();

// Expected output:
// 5
// 4611686018427387904
// -9223372036854775808
// 16
// 4
// -4
// -1
// -1
