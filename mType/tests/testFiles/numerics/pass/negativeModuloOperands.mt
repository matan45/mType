// Test: modulo with negative operands uses C-style truncated semantics
// (result takes the sign of the dividend).
function main(): void {
    print(-7 % 3);
    print(7 % -3);
    print(-7 % -3);
    print(9 % -5);
    print(-9 % 5);
    print(-9 % -5);
    print(0 % 5);
}
main();

// Expected output:
// -1
// 1
// -1
// 4
// -4
// -4
// 0
