// Test: int literals at the int32 boundary parse and print correctly.
// Literals ABOVE INT32_MAX are covered by the MYT-385 canaries.
function main(): void {
    print(2147483647);
    print(-2147483647);
    print(2000000000 + 147483647);
    print(0 - 2147483647 - 1);
}
main();

// Expected output:
// 2147483647
// -2147483647
// 2147483647
// -2147483648
