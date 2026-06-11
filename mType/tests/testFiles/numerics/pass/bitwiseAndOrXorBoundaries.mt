// Test: AND/OR/XOR identities with all-ones (-1) and all-zeros operands.
function main(): void {
    print(-1 & 255);
    print(0 & 255);
    print(0 | 255);
    print(-1 | 255);
    print(255 ^ 255);
    print(255 ^ 0);
    print(255 ^ -1);
    print(-1 ^ -1);
}
main();

// Expected output:
// 255
// 0
// 255
// -1
// 0
// 255
// -256
// 0
