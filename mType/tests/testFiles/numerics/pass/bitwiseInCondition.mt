// Test: bitwise results used directly in if/while conditions.
// Operands are variables on purpose: constant-vs-constant comparisons in
// conditions are the MYT-386 canaries, not this test.
function main(): void {
    int mask = 12;
    int v = 8;
    if ((v & mask) != 0) {
        print("bit set");
    } else {
        print("bit clear");
    }
    int w = 3;
    if ((w & mask) != 0) {
        print("bit set");
    } else {
        print("bit clear");
    }
    while ((v & 1) == 0) {
        v = v >> 1;
    }
    print(v);
}
main();

// Expected output:
// bit set
// bit clear
// 1
